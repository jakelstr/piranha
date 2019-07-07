#ifndef CONSTRUCTED_STRING_NODE_OUTPUT_H
#define CONSTRUCTED_STRING_NODE_OUTPUT_H

#include <string_node_output.h>

namespace piranha {

	class ConstructedStringNodeOutput : public StringNodeOutput {
	public:
		ConstructedStringNodeOutput();
		virtual ~ConstructedStringNodeOutput();

		virtual void fullCompute(void *target) const;

		pNodeInput *getStringConnection() { return &m_stringInput; }
		const NodeOutput *getStringOutput() { return m_stringInput; }

		virtual void registerInputs();

	protected:
		pNodeInput m_stringInput;
	};

} /* namespace piranha */

#endif /* CONSTRUCTED_VECTOR_NODE_OUTPUT_H */
