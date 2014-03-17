#include <limits.h>
#include "../rbf/mem_util.h"
#include "qe.h"
#include <map>

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

void Iterator::readAttribute(Value &val, const vector<Attribute> &attrs, const string &attrName, const void *data)
{
	int buffer_index = 0;
	int dest_index = 0;
	vector<Attribute>::const_iterator attribute_it = attrs.begin();

	while ( attribute_it != attrs.end())
	{
		if( (*attribute_it).type == TypeVarChar )
		{
			//if attribute name equals to lhs attr name
			if ((*attribute_it).name == attrName) {
				//copy length and string to lhsVal
				dest_index = 0;
				copyStrBetweenBuffer(val.data, dest_index, data, buffer_index);
				val.type = (*attribute_it).type;
			} else {
				int zeroOffset = 0;
				//length of string
				buffer_index += sizeof(int) + readIntFromBuffer(data, zeroOffset);
			}
		}
		else if( (*attribute_it).type == TypeReal )
		{
			if ((*attribute_it).name == attrName) {
				//copy length and string to lhsVal
				dest_index = 0;
				copyFloatBetweenBuffer(val.data, dest_index, data, buffer_index);
				val.type = (*attribute_it).type;
			} else 
				buffer_index += sizeof(float);
		}
		else // INT
		{
			if ((*attribute_it).name == attrName) {
				//copy length and string to lhsVal
				dest_index = 0;
				copyFloatBetweenBuffer(val.data, dest_index, data, buffer_index);
				val.type = (*attribute_it).type;
			} else
				buffer_index += sizeof(int);
		}
		++attribute_it;
	}
}

RC Filter::getNextTuple(void *data)
{
	//iterate until we get a valid answer
	while(this->input->getNextTuple(data) != QE_EOF) {

		vector<Attribute> attrs;
		this->input->getAttributes(attrs);

		//value for left hand side and right hand side
		Value lhsVal;
		lhsVal.data = malloc(200);
		readAttribute(lhsVal, attrs, this->condition.lhsAttr, data);

		Value rhsVal;
		//the case when right hand side is attribute
		if(this->condition.bRhsIsAttr) {
			rhsVal.data = malloc(200);
			readAttribute(rhsVal, attrs, this->condition.rhsAttr, data);
		} else {
			rhsVal = this->condition.rhsValue;
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

void Iterator::projectRecord(void *dest, const void *src, const vector<Attribute> &recordDescriptor, const vector<string> &projAttrs)
{
	int buffer_index = 0;
	int dest_index = 0;

	vector<Attribute>::const_iterator attribute_it = recordDescriptor.begin();
	vector<string>::const_iterator proj_it = projAttrs.begin();

	while ( attribute_it != recordDescriptor.end() && proj_it != projAttrs.end() )
	{
		if( (*attribute_it).type == TypeVarChar )
		{
			//if attribute name equals to target
			if ((*attribute_it).name == *(proj_it))
			{
				//copy length and string to dest
				copyStrBetweenBuffer(dest, dest_index, src, buffer_index);
				++proj_it;
			}
			else {
				int zeroOffset = 0;
				//length of string
				buffer_index += sizeof(int) + readIntFromBuffer(src, zeroOffset);
			}
		}
		else if( (*attribute_it).type == TypeReal )
		{
			if ((*attribute_it).name == *(proj_it))
			{
				copyFloatBetweenBuffer(dest, dest_index, src, buffer_index);
				++proj_it;
			} 
			else 
				buffer_index += sizeof(float);
		}
		else // INT
		{
			if ((*attribute_it).name == *(proj_it))
			{
				copyIntBetweenBuffer(dest, dest_index, src, buffer_index);
				proj_it++;
			}
			else
				buffer_index += sizeof(int);
		}
		++attribute_it;
	}
}

RC Project::getNextTuple(void *data) 
{
	void* inputData = malloc(200);

	if(this->input->getNextTuple(inputData) != QE_EOF) {
		vector<Attribute> attrs;
		this->input->getAttributes(attrs);
		projectRecord(data, inputData, attrs, this->attrNames);
		free(inputData);
		return RC_SUCCESS;
	} else {
		return QE_EOF;
	}

};

void Project::getAttributes(vector<Attribute> &attrs) const
{
	vector<Attribute> inputAttrs;
	this->input->getAttributes(inputAttrs);

	vector<Attribute>::const_iterator attr_it = inputAttrs.begin();
	vector<string>::const_iterator proj_it = this->attrNames.begin();

	while ( attr_it != inputAttrs.end() && proj_it != this->attrNames.end() )
	{
		//if attribute name equals to target
		if ((*attr_it).name == *(proj_it)) {
			attrs.push_back(*attr_it);
			proj_it++;
		}
		attr_it++;
	}
};

RC Aggregate::getNextTuple(void *data)
{
	if ( !this->isGrouped )
	{
		if(this->aggAttr.type == TypeInt)
		{	return getNextTupleByType<int, int>(data);}
		else if (this->aggAttr.type == TypeReal)
		{	return getNextTupleByType<float, int>(data);}
		else
		{	return QE_EOF;}
	}
	else
	{
		if(this->aggAttr.type == TypeInt && this->gAttr.type == TypeInt) 
		{
			return getNextTupleByType<int, int>(data);
		} 
	    else if(this->aggAttr.type == TypeInt && this->gAttr.type == TypeReal) 
	    {
		    return getNextTupleByType<int, float>(data);
		} 
		else if(this->aggAttr.type == TypeInt && this->gAttr.type == TypeVarChar) 
		{
			return getNextTupleByType<int, string>(data);
		} 
		else if(this->aggAttr.type == TypeReal && this->gAttr.type == TypeInt) 
		{
			return getNextTupleByType<float, int>(data);
		} 
		else if(this->aggAttr.type == TypeReal && this->gAttr.type == TypeReal) 
		{
			return getNextTupleByType<float, float>(data);
		} 
		else if(this->aggAttr.type == TypeReal && this->gAttr.type == TypeVarChar) 
		{
			return getNextTupleByType<float, string>(data);
		} 
		else
		{
			return QE_EOF;
		}
	}
	return QE_EOF;
};

template <class T, class T2>
RC Aggregate::getNextTupleByType(void *data)
{
	if(done) return QE_EOF;

	static map<T2, vector<T>> gAttr_hash;
	static bool ini = true;

	if(!isGrouped) {
		void *inputData = malloc(200);
		//initialization
		T max = numeric_limits<T>::min();
		T min = numeric_limits<T>::max();
		T sum = 0;
		int count = 0;

		while(this->input->getNextTuple(inputData) != QE_EOF) {

			Value val;
			val.data = malloc(200);

			vector<Attribute> attrs;
			this->input->getAttributes(attrs);
			//read attribute from record
			readAttribute(val, attrs, this->aggAttr.name, inputData);
			
			//convert buffer to specific data
			int offset = 0;
			T num = reader<T>::readFromBuffer(val.data, offset);
			free(val.data);

			//increase relavent values
			sum += num;
			count++;
			if(num > max) max = num;
			if(num < min) min = num;

		}

		done = true;

		int offset = 0;
		if(this->op == MIN) {
			reader<T>::writeToBuffer(data, offset, min);
		} else if(this->op == MAX) {
			reader<T>::writeToBuffer(data, offset, max);
		} else if(this->op == SUM) {
			reader<T>::writeToBuffer(data, offset, sum);
		} else if(this->op == COUNT) {//special case for count
			reader<int>::writeToBuffer(data, offset, count);
		} else if(this->op == AVG) {//special case for avg
			reader<float>::writeToBuffer(data, offset, (sum / (float)count) );
		}

		free(inputData);

		return RC_SUCCESS;
	} else {
		if (ini)
		{
			void *inputData = malloc(200);
	        T max = numeric_limits<T>::min();
	        T min = numeric_limits<T>::max();
	        T sum = 0;
	        int count = 1;

			while(this->input->getNextTuple(inputData) != QE_EOF){
		
				Value gval;
	            gval.data = malloc(200);
                Value val;
		        val.data = malloc(200);

		        vector<Attribute> attrs;
		        this->input->getAttributes(attrs);
		        //read attribute from record
		        readAttribute(val, attrs, this->aggAttr.name, inputData);
			
	            //convert buffer to specific data
		        int offset = 0;
		        T num = reader<T>::readFromBuffer(val.data, offset);
		        free(val.data);


	            readAttribute(gval, attrs, this->gAttr.name, inputData);
			
	            //convert buffer to specific data
		        int offset_group = 0;
	            T2 group = reader<T2>::readFromBuffer(gval.data, offset_group);
	            free(gval.data);

		        if( gAttr_hash.find(group) == gAttr_hash.end() )
				{
					vector<T> stat;
			        stat.push_back(num); //sum
			        stat.push_back(1); //count
			        stat.push_back(num); //max
			        stat.push_back(num); //min
			        gAttr_hash.insert(make_pair(group, stat));
				}   
	            else
				{
			        (gAttr_hash[group])[0] += num;
			        (gAttr_hash[group])[1] ++;
		            if(num > (gAttr_hash[group])[2]) 
						(gAttr_hash[group])[2] = num;
					if(num < (gAttr_hash[group])[3]) 
				        (gAttr_hash[group])[3] = num;
				}
				

			}
			map<T2, vector<T>>::iterator it = gAttr_hash.begin();

		    ini = false;
		    free(inputData);
		    }

			if ( !(gAttr_hash.empty()) )
			{
				map<T2, vector<T>>::iterator it = gAttr_hash.begin();
				int offset = 0;
                reader<T2>::writeToBuffer(data, offset, it->first); 
			    

				if(this->op == MIN) {
					reader<T>::writeToBuffer(data, offset, (it->second)[3]);
		        } else if(this->op == MAX) {
			        reader<T>::writeToBuffer(data, offset, (it->second)[2]);
		        } else if(this->op == SUM) {
			        reader<T>::writeToBuffer(data, offset, (it->second)[0]);
		        } else if(this->op == COUNT) {//special case for count
			        reader<int>::writeToBuffer(data, offset, (it->second)[1]);
		        } else if(this->op == AVG) {//special case for avg
			        reader<float>::writeToBuffer(data, offset, ((it->second)[0] / (float)((it->second)[1])) );
				}
			    
				gAttr_hash.erase( gAttr_hash.begin() );
				if( gAttr_hash.empty() )
				{
					ini = true;
				    done = true;
				}

				return RC_SUCCESS;
			}
		
	}
	

}


NLJoin::NLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages)
{
	this->iter = leftIn;
	this->scan = rightIn;
	this->cond = condition;
	this->numOfPages = numPages;

	leftIn->getAttributes(leftAttrs);
	rightIn->getAttributes(rightAttrs);

	vector<Attribute>::iterator attrs_it = leftAttrs.begin();
	while (attrs_it != leftAttrs.end())
	{
		this->attrs.push_back(*attrs_it);
		attrs_it++;
	}
	attrs_it = rightAttrs.begin();
	while (attrs_it != rightAttrs.end())
	{
		this->attrs.push_back(*attrs_it);
		attrs_it++;
	}
}


void NLJoin::getAttributes(vector<Attribute> &attrs) const
{
	attrs.clear();
	attrs = this->attrs;
}
bool NLJoin::isConditionTrue(const Value &lhsVal, const Value &rhsVal, CompOp op)
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
bool NLJoin::isConditionTrueByType(const T &lhsVal, const T &rhsVal, CompOp op)
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
int getAttributeLength(const vector<Attribute> &attrs, const void *data)
{
	int buffer_index = 0;
	int dest_index = 0;
	int offset = 0;
	vector<Attribute>::const_iterator attribute_it = attrs.begin();

	while ( attribute_it != attrs.end())
	{
		if( (*attribute_it).type == TypeVarChar )
		{
			//string length
			int *pStrLen = (int*)malloc(sizeof(int));

		    memcpy(pStrLen, (char*)data + offset, sizeof(int));
		    offset += sizeof(int);

		    //string
		    char *str = (char*)malloc(*pStrLen + 1);
			memcpy(str, (char*)data + offset, *pStrLen);
		    offset += *pStrLen;

			free(pStrLen);
		}
		else if( (*attribute_it).type == TypeReal )
		{
			offset += sizeof(int);
		}
		else // INT
		{
			offset += sizeof(float);
		}
		++attribute_it;
	}
	return offset;
}
RC NLJoin::getNextTuple(void *data)
{
	// Buffer size and character buffer size
    const unsigned bufSize = 200;

	// Buffer data that holds the left/right tuple
	void *leftData = malloc(bufSize);
	void *rightData = malloc(bufSize);

	int offsetL = 0, offsetR = 0, desIndex = 0;
	static int count = 0;


	// TypeInt = 0, TypeReal, TypeVarChar
	while ( iter->getNextTuple(leftData) != QE_EOF)
	{
	    // value of left/right value
	    Value lhsVal;
	    lhsVal.data = malloc(200);
	    readAttribute(lhsVal, leftAttrs, this->cond.lhsAttr, leftData);
	    Value rhsVal = this->cond.rhsValue;
		int lengthLeft = getAttributeLength(leftAttrs, leftData);

		scan->resetIterator();
		while( scan->getNextTuple(rightData) != QE_EOF)
		{
			rhsVal.data = malloc(200);
		    readAttribute(rhsVal, rightAttrs, this->cond.rhsAttr, rightData);
			int lengthRight = getAttributeLength(rightAttrs, rightData);

			//check type matching
			if(lhsVal.type != rhsVal.type)
			{
				free(lhsVal.data);
				free(rhsVal.data);

				delete leftData;
	            delete rightData;
				return RC_TYPE_MISMATCH;
			}

		    if( isConditionTrue(lhsVal, rhsVal, this->cond.op) ) 
			{
				free(lhsVal.data);
				free(rhsVal.data);
				memcpy(data, leftData, lengthLeft);
				memcpy( (char*)data + lengthLeft, rightData, lengthRight);
                
				delete leftData;
	            delete rightData;
			    
				return RC_SUCCESS;
			} 
			free(rhsVal.data);
		}
		free(lhsVal.data);
	}

	delete leftData;
	delete rightData;
	return QE_EOF;
}
INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition,const unsigned numPages)
{
	this->iter = leftIn;
	this->indexScan = rightIn;
	this->cond = condition;
	this->numOfPages = numPages;

	leftIn->getAttributes(leftAttrs);
	rightIn->getAttributes(rightAttrs);

	vector<Attribute>::iterator attrs_it = leftAttrs.begin();
	while (attrs_it != leftAttrs.end())
	{
		this->attrs.push_back(*attrs_it);
		attrs_it++;
	}
	attrs_it = rightAttrs.begin();
	while (attrs_it != rightAttrs.end())
	{
		this->attrs.push_back(*attrs_it);
		attrs_it++;
	}
}
void INLJoin::getAttributes(vector<Attribute> &attrs) const
{
	attrs.clear();
	attrs = this->attrs;
}
bool INLJoin::isConditionTrue(const Value &lhsVal, const Value &rhsVal, CompOp op)
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
bool INLJoin::isConditionTrueByType(const T &lhsVal, const T &rhsVal, CompOp op)
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
RC INLJoin::getNextTuple(void *data)
{
	// Buffer size and character buffer size
    const unsigned bufSize = 200;

	// Buffer data that holds the left/right tuple
	void *leftData = malloc(bufSize);
	void *rightData = malloc(bufSize);

	int offsetL = 0, offsetR = 0, desIndex = 0;
	static int count = 0;


	// TypeInt = 0, TypeReal, TypeVarChar
	while ( iter->getNextTuple(leftData) != QE_EOF)
	{
	    // value of left/right value
	    Value lhsVal;
	    lhsVal.data = malloc(200);
	    readAttribute(lhsVal, leftAttrs, this->cond.lhsAttr, leftData);
	    Value rhsVal = this->cond.rhsValue;
		int lengthLeft = getAttributeLength(leftAttrs, leftData);

		indexScan->resetIterator();
		while( this->indexScan->getNextTuple(rightData) != QE_EOF)
		{
			rhsVal.data = malloc(200);
		    readAttribute(rhsVal, rightAttrs, this->cond.rhsAttr, rightData);
			int lengthRight = getAttributeLength(rightAttrs, rightData);

			//check type matching
			if(lhsVal.type != rhsVal.type)
			{
				free(lhsVal.data);
				free(rhsVal.data);

				delete leftData;
	            delete rightData;
				return RC_TYPE_MISMATCH;
			}

		    if( isConditionTrue(lhsVal, rhsVal, this->cond.op) ) 
			{
				free(lhsVal.data);
				free(rhsVal.data);
				memcpy(data, leftData, lengthLeft);
				memcpy( (char*)data + lengthLeft, rightData, lengthRight);
                
				delete leftData;
	            delete rightData;
			    
				return RC_SUCCESS;
			}
            
		    free(rhsVal.data);
		}
		free(lhsVal.data);
	}

	delete leftData;
	delete rightData;
	return QE_EOF;
}
