#include "ir_parser_structure.h"

#include "ir_compilation_unit.h"
#include "ir_context_tree.h"

piranha::IrParserStructure::IrReferenceInfo::IrReferenceInfo() {
	newContext = nullptr;
	err = nullptr;

	failed = false;
	reachedDeadEnd = false;
	touchedMainContext = false;
}

piranha::IrParserStructure::IrReferenceInfo::~IrReferenceInfo() {
	/* void */
}

void piranha::IrParserStructure::IrReferenceInfo::reset() {
	newContext = nullptr;
	err = nullptr;

	failed = false;
	reachedDeadEnd = false;
	touchedMainContext = false;
}

piranha::IrParserStructure::IrReferenceQuery::IrReferenceQuery() {
	inputContext = nullptr;
	recordErrors = false;
}

piranha::IrParserStructure::IrReferenceQuery::~IrReferenceQuery() {
	/* void */
}


piranha::IrParserStructure::IrParserStructure() {
	m_parentScope = nullptr;
	m_logicalParent = nullptr;
	m_checkReferences = true;

	m_definitionsResolved = false;
	m_validated = false;

	m_visibility = IrVisibility::DEFAULT;
	m_defaultVisibility = IrVisibility::PRIVATE;
}

piranha::IrParserStructure::~IrParserStructure() {
	/* void */
}

void piranha::IrParserStructure::registerToken(const IrTokenInfo *tokenInfo) {
	if (tokenInfo != nullptr) m_summaryToken.combine(tokenInfo);
}

void piranha::IrParserStructure::registerComponent(IrParserStructure *child) {
	if (child != nullptr) {
		m_summaryToken.combine(child->getSummaryToken());
		child->setParentScope(this);
		child->setLogicalParent(this);

		m_components.push_back(child);
	}
}

piranha::IrParserStructure *piranha::IrParserStructure::resolveName(const std::string &name) const {
	IrParserStructure *local = resolveLocalName(name);
	if (local != nullptr) return local;
	
	if (m_parentScope != nullptr) {
		return m_parentScope->resolveName(name);
	}

	return nullptr;
}

piranha::IrParserStructure *piranha::IrParserStructure::getImmediateReference(const IrReferenceQuery &query, IrReferenceInfo *output) {
	return nullptr;
}

piranha::IrParserStructure *piranha::IrParserStructure::getReference(const IrReferenceQuery &query, IrReferenceInfo *output) {
	IR_RESET(query);

	IrReferenceQuery immediateQuery = query;
	IrReferenceInfo immediateInfo;
	IrParserStructure *immediateReference = getImmediateReference(immediateQuery, &immediateInfo);

	if (immediateInfo.failed) {
		IR_FAIL();
		IR_ERR_OUT(immediateInfo.err);
		return nullptr;
	}

	if (immediateInfo.reachedDeadEnd) {
		IR_DEAD_END();
		return nullptr;
	}

	if (immediateReference != nullptr) {
		IrReferenceInfo nestedInfo;
		IrReferenceQuery nestedQuery;
		nestedQuery.inputContext = immediateInfo.newContext;

		// Error checking is not done on any parent nodes because it's assumed that errors have
		// already been checked/reported
		nestedQuery.recordErrors = false;

		IrParserStructure *fullReference = immediateReference->getReference(nestedQuery, &nestedInfo);

		if (nestedInfo.failed) {
			IR_FAIL();
			IR_ERR_OUT(nestedInfo.err);
			return nullptr;
		}

		if (nestedInfo.reachedDeadEnd) {
			IR_DEAD_END();
			return nullptr;
		}

		IR_INFO_OUT(newContext, nestedInfo.newContext);
		IR_INFO_OUT(touchedMainContext, nestedInfo.touchedMainContext || immediateInfo.touchedMainContext);
		return fullReference;
	}
	else {
		return this;
	}
}

void piranha::IrParserStructure::resolveDefinitions() {
	if (m_definitionsResolved) return;

	// Resolve components
	int componentCount = getComponentCount();
	for (int i = 0; i < componentCount; i++) {
		m_components[i]->resolveDefinitions();
	}

	_resolveDefinitions();

	m_definitionsResolved = true;
}

void piranha::IrParserStructure::checkReferences(IrContextTree *inputContext) {
	// Check components
	int componentCount = getComponentCount();
	for (int i = 0; i < componentCount; i++) {
		m_components[i]->checkReferences(inputContext);
	}

	if (m_checkReferences) {
		IrReferenceQuery query;
		query.inputContext = inputContext;
		query.recordErrors = true;
		IrReferenceInfo info;

		IrParserStructure *reference = getReference(query, &info);

		if (info.err != nullptr) {
			getParentUnit()->addCompilationError(info.err);
		}
	}
}

void piranha::IrParserStructure::checkInstantiation() {
	// Check components
	int componentCount = getComponentCount();
	for (int i = 0; i < componentCount; i++) {
		m_components[i]->checkInstantiation();
	}

	_checkInstantiation();
}

void piranha::IrParserStructure::validate() {
	if (m_validated) return;

	// Validate components
	int componentCount = getComponentCount();
	for (int i = 0; i < componentCount; i++) {
		m_components[i]->validate();
	}

	_validate();

	m_validated = true;
}

void piranha::IrParserStructure::_validate() {
	/* void */
}

void piranha::IrParserStructure::_checkInstantiation() {
	/* void */
}

void piranha::IrParserStructure::writeReferencesToFile(std::ofstream &file, IrContextTree *context, int tabLevel) {
	for (int i = 0; i < tabLevel; i++) {
		file << " ";
	}

	if (m_summaryToken.lineStart != m_summaryToken.lineEnd) {
		file << "L[" << m_summaryToken.lineStart << "-" << m_summaryToken.lineEnd << "]";
	}
	else {
		file << "L[" << m_summaryToken.lineStart << "]";
	}

	file << "[" << m_summaryToken.colStart << "-" << m_summaryToken.colEnd << "]";

	IrReferenceInfo info;
	IrReferenceQuery query;
	query.inputContext = context;
	query.recordErrors = false;

	IrNode *asNode = getAsNode();
	IrParserStructure *immediateReference = getImmediateReference(query, &info);

	if (info.failed) {
		file << " => "; 
		file << "ERROR" << std::endl;
		return;
	}
	else if (immediateReference == nullptr) {
		file << std::endl;
		return;
	}
	else {
		file << " => ";
		immediateReference->writeReferencesToFile(file, info.newContext);
	}
}

void piranha::IrParserStructure::_resolveDefinitions() {
	/* void */
}

piranha::IrParserStructure *piranha::IrParserStructure::resolveLocalName(const std::string &name) const {
	return nullptr;
}

bool piranha::IrParserStructure::allowsExternalAccess() const {
	IrVisibility visibility = (m_visibility == IrVisibility::DEFAULT) ? m_defaultVisibility : m_visibility;
	return visibility == IrVisibility::PUBLIC;
}

piranha::IrCompilationUnit *piranha::IrParserStructure::getParentUnit() const {
	if (m_parentUnit == nullptr) return m_logicalParent->getParentUnit();
	else return m_parentUnit;
}
