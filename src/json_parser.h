#pragma once

#include <map>
#include <vector>

typedef enum {
	JSONSTATE_NONE = -1,
	JSONSTATE_KEY = 0,
	JSONSTATE_VALUE = 1,
	JSONSTATE_COMPLETE = 2,
} JSONSTATE;

typedef enum {
	JSONVALUE_NONE = -1,
	JSONVALUE_OBJECT = 0,
	JSONVALUE_ARRAY = 1,
	JSONVALUE_STRING = 2,
	JSONVALUE_NUMBER = 3,
	JSONVALUE_BOOL = 4,
	JSONVALUE_NULL = 5,
} JSONVALUE;

struct JsonObjectValue;

struct JsonKeyPair
{
	std::string key;
	JsonObjectValue *value;

	void clear(void);
	JsonKeyPair(void);
	void print(void);
};

class JsonObjectMapper
{
public:
	virtual void sync(JsonObjectValue *objval) = 0;
};

class JsonObject
{
	std::map<std::string, JsonObjectValue *> mapEntries;

public:
	JsonObject(void) {}
	~JsonObject(void) {}

	int insert(std::string key, JsonObjectValue *value);
	JsonObjectValue* find(std::string key);
	void toStr(std::string &str);
	void clear(void);
};

class JsonArray
{
	std::vector<JsonObjectValue *> arEntries;

public:
	JsonArray(void) {}
	~JsonArray(void) {}

	int insert(JsonObjectValue *value);
	JsonObjectValue* at(size_t i) { return arEntries.at(i); }
	size_t size() { return arEntries.size(); }
	void toStr(std::string &str);
	void clear(void);
};

struct JsonObjectValue
{
	JSONVALUE type;
	JSONSTATE state;
	void *value;
	int index_start;
	int index_current;
	JsonKeyPair keypair;
	/* if inout is true, save the value  to this structure
	 * and if inout is false, get the value from this structure
	 */
	bool inout;

	JsonObjectValue(void)
		: type(JSONVALUE_NONE), state(JSONSTATE_NONE), inout(false)
	{}
	void clear(void);
	/* parsing functions */
	int tryInsertPair(std::string &jsonStr);
	int tryGetValue(std::string &jsonStr);
	/* converting to string */
	void toStr(std::string &str);
	/* find matching value for specifed key */
	JsonObjectValue* find(std::string key);

	/* sync functions for various types */
	void sync(JsonObjectMapper &value);
	template<typename T>
	void sync(std::vector<T> &arValues)
	{
		if(this->type == JSONVALUE_ARRAY) {
			JsonArray *arr = static_cast<JsonArray *>(this->value);
			if(!arr) {
				return;
			}
			if(this->inout == false) {
				size_t size = arr->size();
				for(size_t idx = 0; idx < size; idx++) {
					JsonObjectValue *objval = NULL;
					T value;
					objval = arr->at(idx);
					objval->inout = this->inout;
					objval->sync(value);
					arValues.push_back(value);
				}
			}
			else {
				size_t size = arValues.size();
				for(size_t idx = 0; idx < size; idx++) {
					T &value = arValues.at(idx);
					JsonObjectValue *objval = new JsonObjectValue;
					objval->inout = this->inout;
					objval->sync(value);
					arr->insert(objval);
				}
			}
		}
		/* the value is initialized for newly created obj */
		else if(this->type == JSONVALUE_NONE && inout == true){
			this->type = JSONVALUE_ARRAY;
			this->value = new JsonArray;
			JsonArray *arr = static_cast<JsonArray *>(this->value);
			size_t size = arValues.size();
			for(size_t idx = 0; idx < size; idx++) {
				T &value = arValues.at(idx);
				JsonObjectValue *objval = new JsonObjectValue;
				objval->inout = this->inout;
				objval->sync(value);
				arr->insert(objval);
			}
		}
	}

	void sync(std::string &value);
	void sync(int &value);
	void sync(bool &value);
	
	template<typename T>
	void sync(std::string key, T &value)
	{
		JsonObject *obj = NULL;
		JsonObjectValue *objval = NULL;

		if(this->type != JSONVALUE_OBJECT) {
			return;
		}
		obj = static_cast<JsonObject *>(this->value);
		if(!obj) {
			return;
		}

		objval = obj->find(key);
		if(objval){
			objval->inout = this->inout;
			objval->sync(value);
		}
		/* to save value, new objval is created */
		else if(this->inout == true){
			objval = new JsonObjectValue;
			if(objval) { 
				objval->inout = this->inout;
				objval->sync(value);
				obj->insert(key, objval);
			}
		}
	}
};

class JsonParser
{
	JsonObjectValue *m_root;
public:
	JsonParser(void);
	~JsonParser(void);

	int parse(std::string jsonStr);
	void syncToObject(JsonObjectMapper &mapper);
	JsonObjectValue* search(std::string key);
};

class JsonMaker
{
	JsonObjectValue *m_root;
public:
	JsonMaker(void);
	~JsonMaker(void);

	int make(std::string &jsonStr);
	void syncFromObject(JsonObjectMapper &mapper);
};

bool JsonRead(JsonObjectMapper &mapper, std::string jsonStr);
bool JsonWrite(JsonObjectMapper &mapper, std::string &jsonStr);
