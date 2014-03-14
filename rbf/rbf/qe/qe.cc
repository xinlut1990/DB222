
#include "qe.h"

Filter::Filter(Iterator* input, const Condition &condition): input(input), condition(condition) {

}

bool Filter::isConditionTrue(const Value &lhsVal, const Value &rhsVal, CompOp op)
{
	int offset = 0;

	if(lhsVal.type == TypeInt) {
		offset = 0;
		int lhs = reader<int>::readFromBuffer(lhsVal.data, offset);
		offset = 0;
		int rhs = reader<int>::readFromBuffer(rhsVal.data, offset);

		return isConditionTrueByType<int>(lhs, rhs, op);

	} else if(lhsVal.type == TypeReal) {
		offset = 0;
		float lhs = reader<float>::readFromBuffer(lhsVal.data, offset);
		offset = 0;
		float rhs = reader<float>::readFromBuffer(rhsVal.data, offset);

		return isConditionTrueByType<float>(lhs, rhs, op);

	} else if(lhsVal.type == TypeVarChar) {
		offset = 0;
		string lhs = reader<string>::readFromBuffer(lhsVal.data, offset);
		offset = 0;
		string rhs = reader<string>::readFromBuffer(rhsVal.data, offset);

		return isConditionTrueByType<string>(lhs, rhs, op);

	} else {
		return false;
	}
}

template <class T>
bool Filter::isConditionTrueByType(const T &lhsVal, const T &rhsVal, CompOp op)
{
	if(op == EQ_OP) {
		return lhsVal == rhsVal;
	} else if(op == LT_OP) {
		return lhsVal < rhsVal;
	} else if(op == GT_OP) {
		return lhsVal > rhsVal;
	} else if(op == LE_OP) {
		return lhsVal <= rhsVal;
	} else if(op == GE_OP) {
		return lhsVal >= rhsVal;
	} else if(op == NE_OP) {
		return lhsVal != rhsVal;
	} else if(op == NO_OP) {
		return lhsVal == rhsVal;
	} else {
		return false;
	}

}

// ... the rest of your implementations go here
RC Filter::getNextTuple(void *data)
{
	while(this->input->getNextTuple(data) != QE_EOF) {

		vector<Attribute> attrs;
		this->input->getAttributes(attrs);

		int buffer_index = 0;
		int dest_index = 0;

		Value lhsVal;
		lhsVal.data = malloc(200);
		Value rhsVal = this->condition.rhsValue;
		if(this->condition.bRhsIsAttr) {
			rhsVal.data = malloc(200);
		}

		vector<Attribute>::const_iterator attribute_it = attrs.begin();

		while ( attribute_it != attrs.end())
		{
			if( (*attribute_it).type == TypeVarChar )
			{
				//if attribute name equals to lhs attr name
				if ((*attribute_it).name == this->condition.lhsAttr) {
					//copy length and string to lhsVal
					dest_index = 0;
					copyStrBetweenBuffer(lhsVal.data, dest_index, data, buffer_index);
					lhsVal.type = (*attribute_it).type;
				} else if (this->condition.bRhsIsAttr && (*attribute_it).name == this->condition.rhsAttr) {

					dest_index = 0;
					copyStrBetweenBuffer(rhsVal.data, dest_index, data, buffer_index);
					rhsVal.type = (*attribute_it).type;
				} else {
					int zeroOffset = 0;
					//length of string
					buffer_index += sizeof(int) + readIntFromBuffer(data, zeroOffset);
				}
			}
			else if( (*attribute_it).type == TypeReal )
			{
				if ((*attribute_it).name == this->condition.lhsAttr) {
					//copy length and string to lhsVal
					dest_index = 0;
					copyFloatBetweenBuffer(lhsVal.data, dest_index, data, buffer_index);
					lhsVal.type = (*attribute_it).type;
				} else if (this->condition.bRhsIsAttr && (*attribute_it).name == this->condition.rhsAttr) {

					dest_index = 0;
					copyFloatBetweenBuffer(rhsVal.data, dest_index, data, buffer_index);
					rhsVal.type = (*attribute_it).type;
				} else 
					buffer_index += sizeof(float);
			}
			else // INT
			{
				if ((*attribute_it).name == this->condition.lhsAttr) {
					//copy length and string to lhsVal
					dest_index = 0;
					copyFloatBetweenBuffer(lhsVal.data, dest_index, data, buffer_index);
					lhsVal.type = (*attribute_it).type;
				} else if (this->condition.bRhsIsAttr && (*attribute_it).name == this->condition.rhsAttr) {

					dest_index = 0;
					copyFloatBetweenBuffer(rhsVal.data, dest_index, data, buffer_index);
					rhsVal.type = (*attribute_it).type;
				} else
					buffer_index += sizeof(int);
			}
			++attribute_it;
		}

		//check type matching
		if(lhsVal.type != rhsVal.type) {
			free(lhsVal.data);
			if(this->condition.bRhsIsAttr) {
				free(rhsVal.data);
			}

			return RC_TYPE_MISMATCH;
		}

		if( isConditionTrue(lhsVal, rhsVal, this->condition.op) ) {
			free(lhsVal.data);
			if(this->condition.bRhsIsAttr) {
				free(rhsVal.data);
			}

			return RC_SUCCESS;
		}

		free(lhsVal.data);
		if(this->condition.bRhsIsAttr) {
			free(rhsVal.data);
		}

	}

	return QE_EOF;
}