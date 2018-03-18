#include "stdafx.h"
#include "json_parser.h"

#include <stdarg.h>
#include <stack>
#include <sstream>
#include <memory>

#define Log_Error(fmt, ...) 
#define Log_Info(fmt, ...) 

static std::string Format(const char *fmt_str, ...)
{
	std::string formatted;
	va_list ap;
	int fn, n = strlen(fmt_str) * 2;

	while(1) {
		formatted.resize(n);
		va_start(ap, fmt_str);
		fn = vsnprintf((char *)formatted.data(), n, fmt_str, ap);
		va_end(ap);
		if(fn < 0) 
			return "";
		else if(fn >= n)
			n += fn - n +1;
		else 
			break;
	}

	return formatted;
}

static int SplitString(std::string str, const char *delimiters, std::vector<std::string> &arSplit)
{
	std::string::size_type pos;
	std::string::size_type prev_pos = 0;
	std::string::size_type next_pos;
	int delim_len; 
	
	delim_len = strlen(delimiters);
	if(delim_len == 0) {
		Log_Error("no delimiter specified.");
		return 1;
	}

	for(; prev_pos < str.length();) {
		std::string token;
		next_pos = str.length();
		for(unsigned int j = 0; j < strlen(delimiters); j++) {
			pos = str.find(delimiters[j], prev_pos); 
			if(pos == std::string::npos)
				continue;
			if(pos < next_pos)
				next_pos = pos;
		}

		token = str.substr(prev_pos, next_pos - prev_pos);
		arSplit.push_back(token);
		Log_Info("token: %s", token.c_str());
		prev_pos = next_pos +1;
	}

	return 0;
}

int JsonObjectValue::tryInsertPair(std::string &jsonStr)
{
	Log_Info("[%s] ++", __FUNCTION__);
	if(this->state != JSONSTATE_VALUE) {
		Log_Error("[%s] incomplete json str: index(%d), length(%d)", __FUNCTION__, this->index_start, jsonStr.length());
		return 1;
	}

	if(this->tryGetValue(jsonStr)) {
		return 1;
	}

	if(this->type == JSONVALUE_OBJECT) {
		JsonObject *obj = static_cast<JsonObject *>(this->value);
		if(obj->insert(this->keypair.key, this->keypair.value)) {
			return 1;
		}
		this->keypair.clear();
	}
	else if(this->type == JSONVALUE_ARRAY) {
		JsonArray *arr = static_cast<JsonArray *>(this->value);
		if(arr->insert(this->keypair.value)) {
			return 1;
		}
		this->keypair.clear();
	}
	Log_Info("[%s] --", __FUNCTION__);
	return 0;
}

int JsonObjectValue::tryGetValue(std::string &jsonStr)
{
	unsigned int i,j;
	std::string tmp;
	std::string str;
	JsonObjectValue *objval = NULL;

	Log_Info("[%s] ++", __FUNCTION__);

	if(this->state == JSONSTATE_KEY && !this->keypair.key.empty()) {
		//Log_Info("[%s]key is alreay parsed: %s", __FUNCTION__, this->keypair.key.c_str());
		//this->keypair.print();
		return 0;
	}
	if(this->state == JSONSTATE_VALUE && this->keypair.value != NULL) {
		//Log_Info("[%s]value is alreay parsed: %d", __FUNCTION__, this->keypair.value->type);
		//this->keypair.print();
		return 0;
	}

	if(jsonStr.length() < this->index_start +1) {
		Log_Error("[%s] incomplete json str: index(%d), length(%d)", __FUNCTION__, this->index_start, jsonStr.length());
		return 1;
	}
	str = jsonStr.substr(this->index_start +1, this->index_current - this->index_start -1);
	Log_Info("[%s] sub string: %s", __FUNCTION__, str.c_str());
	/* trim characters */
	for(i = 0; i < str.length(); i++) {
		if(str.at(i) !=  ' ' && str.at(i) != '\r' && str.at(i) != '\n' && str.at(i) != '\t' )
			break;
	}
	for(j = str.length() -1; j >= 0; j--) {
		if(str.at(j) !=  ' ' && str.at(j) != '\r' && str.at(j) != '\n' && str.at(j) != '\t' )
			break;
	}

	if(this->state == JSONSTATE_KEY) {
		if(str.at(i) != '"' || str.at(j) != '"') {
			Log_Error("[%s]invalid charactor for string: %d(%c), %d(%c)", __FUNCTION__, i, str.at(i), j, str.at(j));
			return 1;
		}
		this->keypair.key = str.substr(i +1, j - i -1);
		goto leave;
	}
	else if(this->state != JSONSTATE_VALUE) {
		Log_Error("[%s]state should be value: %s", __FUNCTION__, jsonStr.c_str());
		return 1;
	}

	Log_Info("[%s] i: %d, j: %d", __FUNCTION__, i, j);
	tmp = str.substr(i, j - i +1);
	Log_Info("[%s] tmp string: %s", __FUNCTION__, tmp.c_str());
	if(str.at(i) == '"' && str.at(j) == '"') {
		objval = new JsonObjectValue;
		objval->type = JSONVALUE_STRING;
		objval->value = new std::string(tmp.substr(1, tmp.length() -2));
	}
	else if(tmp == "true") {
		objval = new JsonObjectValue;
		objval->type = JSONVALUE_BOOL;
		objval->value = new bool(true);
	}
	else if(tmp == "false") {
		objval = new JsonObjectValue;
		objval->type = JSONVALUE_BOOL;
		objval->value = new bool(false);
	}
	else if(tmp == "null") {
		objval = new JsonObjectValue;
		objval->type = JSONVALUE_NULL;
		objval->value = NULL;
	}
	else {
		int num = atoi(tmp.c_str());
		if(num == 0 && errno == EINVAL) {
			Log_Error("invalid string: %s", str.c_str());
			return 1;
		}
		//Log_Info("[%s] num: %d", __FUNCTION__, num);
		objval = new JsonObjectValue;
		objval->type = JSONVALUE_NUMBER;
		objval->value = new int(num);
	}

	this->keypair.value = objval;

leave:
	Log_Info("[%s] --", __FUNCTION__);
	return 0;
}

JsonObjectValue* JsonObjectValue::find(std::string key)
{
	JsonObjectValue *foundObjVal;
	if(this->type == JSONVALUE_OBJECT) {
		JsonObject *obj = static_cast<JsonObject *>(this->value);
		foundObjVal = obj->find(key);
		return foundObjVal;
	}
	return NULL;
}

void JsonObjectValue::toStr(std::string &str)
{
	switch(this->type) {
	case JSONVALUE_OBJECT: 
	{
		JsonObject *obj = static_cast<JsonObject *>(this->value);
		obj->toStr(str);
	}
		break;
	case JSONVALUE_ARRAY: 
	{
		JsonArray *arr = static_cast<JsonArray *>(this->value);
		arr->toStr(str);
	}
		break;
	case JSONVALUE_STRING: 
	{
		std::string *val = static_cast<std::string *>(this->value);
		str = Format("\"%s\"", val->c_str());
		break;
	}
	case JSONVALUE_NUMBER: 
	{
		int *val = static_cast<int *>(this->value);
		str = Format("%d", *val);
	}
		break;
	case JSONVALUE_BOOL: 
	{
		bool *val = static_cast<bool *>(this->value);
		str = (*val) ? "true" : "false";
	}
		break;
	case JSONVALUE_NULL: 
		str = "null";
		break;
	default:
		;
	}
}

void JsonObjectValue::sync(JsonObjectMapper &value)
{
	if(this->type == JSONVALUE_OBJECT) {
		JsonObject *obj = static_cast<JsonObject *>(this->value);
		if(!obj) {
			return;
		}
		value.sync(this);
	}
	else if(this->type == JSONVALUE_NONE && this->inout == true){
		this->type = JSONVALUE_OBJECT;
		this->value = new JsonObject;
		value.sync(this);
	}
}

void JsonObjectValue::sync(std::string &value)
{
	if(this->type == JSONVALUE_STRING) {
		std::string *pstr = static_cast<std::string *>(this->value);
		if(!pstr) {
			return;
		}
		if(this->inout == false) {
			value = *pstr;
		}
		else {
			*pstr = value;
		}
	}
	/* the value is initialized for newly created obj */
	else if(this->type == JSONVALUE_NONE && this->inout == true){
		this->type = JSONVALUE_STRING;
		this->value = new std::string(value);
	}
}
void JsonObjectValue::sync(int &value)
{
	if(this->type == JSONVALUE_NUMBER) {
		int *pnum = static_cast<int *>(this->value);
		if(!pnum) {
			return;
		}
		if(this->inout == false) {
			value = *pnum;
		}
		else {
			*pnum = value;
		}
	}
	/* the value is initialized for newly created obj */
	else if(this->type == JSONVALUE_NONE && this->inout == true){
		this->type = JSONVALUE_NUMBER;
		this->value = new int(value);
	}
}
void JsonObjectValue::sync(bool &value)
{
	if(this->type == JSONVALUE_BOOL) {
		bool *p = static_cast<bool *>(this->value);
		if(!p) {
			return;
		}
		if(this->inout == false) {
			value = *p;
		}
		else {
			*p = value;
		}
	}
	/* the value is initialized for newly created obj */
	else if(this->type == JSONVALUE_NONE && this->inout == true){
		this->type = JSONVALUE_BOOL;
		this->value = new bool(value);
	}
}

JsonKeyPair::JsonKeyPair(void)
	: key(""), value(NULL)
{
}

void JsonKeyPair::print(void)
{
	std::string value;
	JsonObjectValue *objval = this->value;
	switch(objval->type) {
	case JSONVALUE_OBJECT: 
		value = "OBJECT";
		break;
	case JSONVALUE_ARRAY: 
		value = "ARRAY";
		break;
	case JSONVALUE_NUMBER: 
		{
		int *val = static_cast<int *>(objval->value);
		value = Format("%d", *val);
		}
		break;

	case JSONVALUE_BOOL: 
		{
		bool *val = static_cast<bool *>(objval->value);
		value = (*val) ? "true" : "false";
		}
		break;
	case JSONVALUE_STRING: 
		{
		std::string *val = static_cast<std::string *>(objval->value);
		value = *val;
		}
		break;
	default:
		value = "N/A";
	}// switch(this->type) 
	Log_Info("key: %s, value: %s", this->key.c_str(), value.c_str());
}
	
void JsonKeyPair::clear(void)
{
	this->key.clear();
	this->value = NULL;
}

void JsonObjectValue::clear(void)
{
	if(this->value == NULL)
		return;
	int i = 0;
	switch(this->type) {
	case JSONVALUE_OBJECT: 
		{
		JsonObject *obj = static_cast<JsonObject *>(this->value);
		obj->clear();
		delete obj;
		}
		break;
	case JSONVALUE_ARRAY: 
		{
		JsonArray *arr = static_cast<JsonArray *>(this->value);
		arr->clear();
		delete arr;
		}
		break;
	case JSONVALUE_BOOL: 
		{
		bool *val = static_cast<bool *>(this->value);
		delete val;
		}
		break;
	case JSONVALUE_STRING: 
		{
		std::string *val = static_cast<std::string *>(this->value);
		delete val;
		}
		break;
	default:
		;
	}// switch(this->type) 
	this->keypair.clear();
}

int JsonObject::insert(std::string key, JsonObjectValue *value)
{
	if(key.empty()) {
		Log_Error("syntax error: ',' there is empty key string");
		return 1;
	}
	if(value == NULL) {
		Log_Error("syntax error: ',' there is no value in this keypair");
		return 1;
	}
	mapEntries.insert(std::make_pair(key, value));
	return 0;
}

JsonObjectValue* JsonObject::find(std::string key)
{
	JsonObjectValue *foundObjVal = NULL;
	std::map<std::string, JsonObjectValue *>::iterator it;

	it = mapEntries.find(key);
	if(it != mapEntries.end()) {
		foundObjVal = it->second;
	}

	return foundObjVal;
}

void JsonObject::toStr(std::string &str)
{
	std::stringstream ss;
	ss << "{"; 
	for(std::map<std::string, JsonObjectValue *>::iterator it = mapEntries.begin(); 
			it != mapEntries.end(); it++) {
		std::string key = it->first;
		std::string valStr;
		JsonObjectValue *objval = it->second;
		objval->toStr(valStr);
		if( it != mapEntries.begin()) {
			ss << ",";
		}
		ss << "\"" << it->first << "\":" << valStr;
	}
	ss << "}"; 

	str = ss.str();
}

void JsonObject::clear(void)
{
	for(std::map<std::string, JsonObjectValue *>::iterator it = mapEntries.begin(); 
			it != mapEntries.end(); it++) {
		JsonObjectValue *objval = it->second;
		objval->clear();
		delete objval;
	}
	mapEntries.clear();
}

int JsonArray::insert(JsonObjectValue *value)
{
	if(value == NULL) {
		Log_Error("syntax error: ',' there is no value in this keypair");
		return 1;
	}
	arEntries.push_back(value);
	return 0;
}

void JsonArray::toStr(std::string &str)
{
	std::stringstream ss;
	ss << "["; 
	for(std::vector<JsonObjectValue *>::iterator it = arEntries.begin(); 
			it != arEntries.end(); it++) {
		JsonObjectValue *objval = *it;
		std::string valStr;
		objval->toStr(valStr);
		if( it != arEntries.begin()) {
			ss << ",";
		}
		ss << valStr;
	}
	ss << "]"; 

	str = ss.str();
}

void JsonArray::clear(void)
{
	for(std::vector<JsonObjectValue *>::iterator it = arEntries.begin(); 
			it != arEntries.end(); it++) {
		JsonObjectValue *objval = *it;
		objval->clear();
		delete objval;
	}
	arEntries.clear();
}

JsonParser::JsonParser(void)
	: m_root(NULL)
{
}

JsonParser::~JsonParser(void)
{
	if(m_root) {
		m_root->clear();
	}
}

int JsonParser::parse(std::string jsonStr)
{
	std::stack<JsonObjectValue *> objectStack;
	JsonObjectValue *root_objval = NULL;
	JsonObjectValue *objval = NULL;
	
	for(unsigned int i=0; i < jsonStr.length(); i++) {
		char c = jsonStr.at(i);

		switch(c) {
		case '{':
			if(objval != NULL) {
				if(objval->state != JSONSTATE_VALUE) {
					Log_Error("object starts not in value or none context, state: %d", objval->state);
					goto err;
				}
				objectStack.push(objval);
				objval = new JsonObjectValue;
			}
			else {
				root_objval = objval = new JsonObjectValue;
			}
			objval->type = JSONVALUE_OBJECT;
			objval->value = new JsonObject;
			objval->state = JSONSTATE_KEY;
			objval->index_start = i;

			break;
		case '}':
			if(objval == NULL) {
				Log_Error("no object starts");
				goto err;
			}
			if(objval->type != JSONVALUE_OBJECT) {
				Log_Error("not in object context");
				goto err;
			}

			objval->index_current = i;
			if(objval->tryInsertPair(jsonStr)) {
				goto err;
			}

			objval->state = JSONSTATE_COMPLETE;

			Log_Info("[%s] %c, stack size: %d", __FUNCTION__, c, objectStack.size());
			if(!objectStack.empty()) {
				JsonObjectValue *parent = NULL;
				parent = objectStack.top();
				parent->keypair.value = objval;
				objectStack.pop();
				objval = parent;
			}

			break;
		case '[':
			if(objval != NULL) {
				if(objval->state != JSONSTATE_VALUE) {
					Log_Error("object starts not in value or none context, state: %d", objval->state);
					goto err;
				}
				objectStack.push(objval);
				objval = new JsonObjectValue;
			}
			else {
				root_objval = objval = new JsonObjectValue;
			}
			objval->type = JSONVALUE_ARRAY;
			objval->value = new JsonArray;
			objval->state = JSONSTATE_VALUE;
			objval->index_start = i;
			break;
		case ']':
			if(objval == NULL) {
				Log_Error("no object starts");
				goto err;
			}
			if(objval->type != JSONVALUE_ARRAY) {
				Log_Error("not in array context");
				goto err;
			}

			objval->index_current = i;
			if(objval->tryInsertPair(jsonStr)) {
				goto err;
			}

			objval->state = JSONSTATE_COMPLETE;

			if(!objectStack.empty()) {
				JsonObjectValue *parent = NULL;
				parent = objectStack.top();
				objectStack.pop();
				parent->keypair.value = objval;
				objval = parent;
			}

			break;
		case ':':
			if(objval->type != JSONVALUE_OBJECT) {
				Log_Error("syntax error: ':' occur not in object");
				goto err;
			}
			if(objval->state != JSONSTATE_KEY) {
				Log_Error("syntax error: ':' occur not in key context");
				goto err;
			}

			objval->index_current = i;
			if(objval->tryGetValue(jsonStr)) {
				goto err;
			}

			if(objval->keypair.key.empty()) {
				Log_Error("syntax error: ':' occur, but no key is specified");
				goto err;
			}

			objval->state = JSONSTATE_VALUE;
			objval->index_start = i;
			break;
		case ',':
			if(objval->type != JSONVALUE_OBJECT && objval->type != JSONVALUE_ARRAY) {
				Log_Error("syntax error: ',' occur not in object/array context");
				goto err;
			}
			if( objval->state != JSONSTATE_VALUE) {
				Log_Error("syntax error: ',' occur not in value context");
				goto err;
			}

			objval->index_current = i;
			if(objval->tryInsertPair(jsonStr)) {
				goto err;
			}
			objval->index_start = i;
			if(objval->type == JSONVALUE_OBJECT)
				objval->state = JSONSTATE_KEY;

			break;
		default:
			;
		}// switch(c) 
	}// for(unsigned int i=0; i < jsonStr.length(); i++) 

	if(objectStack.size() != 0) {
		Log_Error("incomplete json string");
		goto err;
	}

leave:
	if(root_objval) {
		m_root = root_objval;
	}
	return 0;

err:
	if(root_objval) {
		root_objval->clear();
		delete root_objval;
	}
	return 1;
}

void JsonParser::syncToObject(JsonObjectMapper &mapper)
{
	JsonObjectValue *objval = m_root;
	std::string outstr;
	objval->toStr(outstr);
	Log_Info("[%s] json str: %s", __FUNCTION__, outstr.c_str());
	objval->inout = false;
	mapper.sync(objval);
}

JsonObjectValue* JsonParser::search(std::string keys)
{
	if(m_root == NULL) {
		Log_Error("not parsed");
		return NULL;
	}

	std::vector<std::string> arKeys;
	JsonObjectValue *objval = m_root;
	JsonObjectValue *sub_objval = NULL; 

	if(SplitString(keys, ".", arKeys)) {
		Log_Error("splitting string failed");
		return NULL;
	}

	for(int i = 0; i < arKeys.size(); i++) {
		std::string key = arKeys.at(i);
		sub_objval = objval->find(key);
		if(!sub_objval) {
			break;
		}
		objval = sub_objval;
	}
	return objval;
}

JsonMaker::JsonMaker(void)
	: m_root(NULL)
{
}
JsonMaker::~JsonMaker(void)
{
	if(m_root) {
		m_root->clear();
	}
}

void JsonMaker::syncFromObject(JsonObjectMapper &mapper)
{
	JsonObjectValue *objval = NULL;
	if(!m_root) {
		m_root = objval = new JsonObjectValue;
		objval->type = JSONVALUE_OBJECT;
		objval->value = new JsonObject;
	}
	objval->inout = true;
	mapper.sync(objval);
}

int JsonMaker::make(std::string &jsonStr)
{
	if(!m_root) {
		return 1;
	}

	m_root->toStr(jsonStr);

	return 0;
}

bool JsonRead(JsonObjectMapper &mapper, std::string jsonStr)
{
	JsonParser parser;
	if(0 != parser.parse(jsonStr)) {
		return false;
	}

	parser.syncToObject(mapper);

	return true;
}

bool JsonWrite(JsonObjectMapper &mapper, std::string &jsonStr)
{
	JsonMaker maker;

	maker.syncFromObject(mapper);
	if(0 != maker.make(jsonStr)) {
		return false;
	}

	return true;
}
