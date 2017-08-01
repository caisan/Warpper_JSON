#include <iostream>
#include <string>
#include <list>
#include <map>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <fstream>
#include "json_spirit.h"

using namespace std;
using namespace json_spirit;
class JSONObj;
class JSONObjIter {
  typedef map<string, JSONObj *>::iterator map_iter_t;
  map_iter_t cur;
  map_iter_t last;

public:
  JSONObjIter();
  ~JSONObjIter();
  void set(const JSONObjIter::map_iter_t &_cur, const JSONObjIter::map_iter_t &_end);

  void operator++();
  JSONObj *operator*();

  bool end() {
    return (cur == last);
  }
};
class JSONObj
{
  JSONObj *parent;
protected:
  string name; // corresponds to obj_type in XMLObj
  Value data;
  string data_string;
  multimap<string, JSONObj *> children;
  map<string, string> attr_map;
  void handle_value(Value v);

public:

  JSONObj() : parent(NULL){};

  virtual ~JSONObj();

  void init(JSONObj *p, Value v, string n);

  string& get_name() { return name; }
  string& get_data() { return data_string; }
  map<string, string>& get_attr_map() {return attr_map;}
  multimap<string, JSONObj *>& get_children() {return children;}
  bool get_data(const string& key, string *dest);
  JSONObj *get_parent();
  void add_child(string el, JSONObj *child);
  bool get_attr(string name, string& attr);
  JSONObjIter find(const string& name);
  JSONObjIter find_first();
  JSONObjIter find_first(const string& name);
  JSONObj *find_obj(const string& name);

  friend ostream& operator<<(ostream& out, JSONObj& obj); // does not work, FIXME

  bool is_array();
  bool is_object();
  vector<string> get_array_elements();
};

class JSONParser : public JSONObj
{
  int buf_len;
  string json_buffer;
  bool success;
public:
  JSONParser();
  virtual ~JSONParser();
  void handle_data(const char *s, int len);

  bool parse(const char *buf_, int len);
  bool parse(int len);
  bool parse();
  bool parse(const char *file_name);

  const char *get_json() { return json_buffer.c_str(); }
  void set_failure() { success = false; }
};
JSONObjIter::JSONObjIter()
{
}

JSONObjIter::~JSONObjIter()
{
}

void JSONObjIter::set(const JSONObjIter::map_iter_t &_cur, const JSONObjIter::map_iter_t &_last)
{
  cur = _cur;
  last = _last;
}

void JSONObjIter::operator++()
{
  if (cur != last)
    ++cur;
};

JSONObj *JSONObjIter::operator*()
{
  return cur->second;
};

// does not work, FIXME
ostream& operator<<(ostream& out, JSONObj& obj) {
   out << obj.name << ": " << obj.data_string;
   return out;
}

JSONObj::~JSONObj()
{
  multimap<string, JSONObj *>::iterator iter;
  for (iter = children.begin(); iter != children.end(); ++iter) {
    JSONObj *obj = iter->second;
    delete obj;
  }
}


void JSONObj::add_child(string el, JSONObj *obj)
{
  children.insert(pair<string, JSONObj *>(el, obj));
}

bool JSONObj::get_attr(string name, string& attr)
{
  map<string, string>::iterator iter = attr_map.find(name);
  if (iter == attr_map.end())
    return false;
  attr = iter->second;
  return true;
}

JSONObjIter JSONObj::find(const string& name)
{
  JSONObjIter iter;
  map<string, JSONObj *>::iterator first;
  map<string, JSONObj *>::iterator last;
  first = children.find(name);
  if (first != children.end()) {
    last = children.upper_bound(name);
    iter.set(first, last);
  }
  return iter;
}

JSONObjIter JSONObj::find_first()
{
  JSONObjIter iter;
  iter.set(children.begin(), children.end());
  return iter;
}

JSONObjIter JSONObj::find_first(const string& name)
{
  JSONObjIter iter;
  map<string, JSONObj *>::iterator first;
  first = children.find(name);
  iter.set(first, children.end());
  return iter;
}

JSONObj *JSONObj::find_obj(const string& name)
{
  JSONObjIter iter = find(name);
  if (iter.end())
    return NULL;

  return *iter;
}

bool JSONObj::get_data(const string& key, string *dest)
{
  JSONObj *obj = find_obj(key);
  if (!obj)
    return false;

  *dest = obj->get_data();

  return true;
}

/* accepts a JSON Array or JSON Object contained in
 * a JSON Spirit Value, v,  and creates a JSONObj for each
 * child contained in v
 */
void JSONObj::handle_value(Value v)
{
  if (v.type() == obj_type) {
    Object temp_obj = v.get_obj();
    for (Object::size_type i = 0; i < temp_obj.size(); i++) {
      Pair temp_pair = temp_obj[i];
      string temp_name = temp_pair.name_;
      Value temp_value = temp_pair.value_;
      JSONObj *child = new JSONObj;
      child->init(this, temp_value, temp_name);
      add_child(temp_name, child);
    }
  } else if (v.type() == array_type) {
    Array temp_array = v.get_array();
    Value value;

    for (unsigned j = 0; j < temp_array.size(); j++) {
      Value cur = temp_array[j];
      string temp_name;

      JSONObj *child = new JSONObj;
      child->init(this, cur, temp_name);
      add_child(child->get_name(), child);
    }
  }
}

void JSONObj::init(JSONObj *p, Value v, string n)
{
  name = n;
  parent = p;
  data = v;

  handle_value(v);
  if (v.type() == str_type)
    data_string =  v.get_str();
  else
    data_string =  write(v, raw_utf8);
  attr_map.insert(pair<string,string>(name, data_string));
}

JSONObj *JSONObj::get_parent()
{
  return parent;
}

bool JSONObj::is_object()
{
  return (data.type() == obj_type);
}

bool JSONObj::is_array()
{
  return (data.type() == array_type);
}

vector<string> JSONObj::get_array_elements()
{
  vector<string> elements;
  Array temp_array;

  if (data.type() == array_type)
    temp_array = data.get_array();

  int array_size = temp_array.size();
  if (array_size > 0)
    for (int i = 0; i < array_size; i++) {
      Value temp_value = temp_array[i];
      string temp_string;
      temp_string = write(temp_value, raw_utf8);
      elements.push_back(temp_string);
    }

  return elements;
}

JSONParser::JSONParser() : buf_len(0), success(true)
{
}

JSONParser::~JSONParser()
{
}

void JSONParser::handle_data(const char *s, int len)
{
  json_buffer.append(s, len); // check for problems with null termination FIXME
  buf_len += len;
}
// parse the internal json_buffer up to len
bool JSONParser::parse(int len)
{
  string json_string = json_buffer.substr(0, len);
  success = read(json_string, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}
// parse a supplied ifstream, for testing. DELETE ME
bool JSONParser::parse(const char *file_name)
{
  ifstream is(file_name);
  success = read(is, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}
// parse a supplied JSON fragment
bool JSONParser::parse(const char *buf_, int len)
{
  if (!buf_) {
    set_failure();
    return false;
  }

  string json_string(buf_, len);
  success = read(json_string, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}
class JSONDecoder {
public:
  struct err {
    string message;

    err(const string& m) : message(m) {}
  };

  JSONParser parser;

  JSONDecoder(string data_str) {
    if (!parser.parse(data_str.c_str(), data_str.length())) {
      cout << "JSONDecoder::err()" << std::endl;
      throw JSONDecoder::err("failed to parse JSON input");
    }
  }
  template<class T>
  static bool decode_json(const char *name, T& val, JSONObj *obj, bool mandatory = false);

  template<class C>
  static bool decode_json(const char *name, C& container, void (*cb)(C&, JSONObj *obj), JSONObj *obj, bool mandatory = false);

  template<class T>
  static void decode_json(const char *name, T& val, T& default_val, JSONObj *obj);
};

template<class T>
void decode_json_obj(T& val, JSONObj *obj)
{
  val.decode_json(obj);
}
void decode_json_obj(unsigned long long& val, JSONObj *obj);
void decode_json_obj(long long& val, JSONObj *obj);
void decode_json_obj(unsigned long& val, JSONObj *obj);
void decode_json_obj(long& val, JSONObj *obj);
void decode_json_obj(unsigned& val, JSONObj *obj);
void decode_json_obj(int& val, JSONObj *obj);
void decode_json_obj(bool& val, JSONObj *obj);
//class utime_t;
//void decode_json_obj(utime_t& val, JSONObj *obj);
template<class T>
void decode_json_obj(list<T>& l, JSONObj *obj)
{
  l.clear();

  JSONObjIter iter = obj->find_first();

  for (; !iter.end(); ++iter) {
    T val;
    JSONObj *o = *iter;
    decode_json_obj(val, o);
    l.push_back(val);
  }
}

template<class K, class V>
void decode_json_obj(map<K, V>& m, JSONObj *obj)
{
  m.clear();

  JSONObjIter iter = obj->find_first();

  for (; !iter.end(); ++iter) {
    K key;
    V val;
    JSONObj *o = *iter;
    JSONDecoder::decode_json("key", key, o);
    JSONDecoder::decode_json("val", val, o);
    m[key] = val;
  }
}

template<class C>
void decode_json_obj(C& container, void (*cb)(C&, JSONObj *obj), JSONObj *obj)
{
  container.clear();

  JSONObjIter iter = obj->find_first();

  for (; !iter.end(); ++iter) {
    JSONObj *o = *iter;
    cb(container, o);
  }
}
static inline void decode_json_obj(string& val, JSONObj *obj)
{
      val = obj->get_data();

}
template<class T>
bool JSONDecoder::decode_json(const char *name, T& val, JSONObj *obj, bool mandatory)
{
  JSONObjIter iter = obj->find_first(name);
  if (iter.end()) {
    if (mandatory) {
      string s = "missing mandatory field " + string(name);
      throw err(s);
    }
    val = T();
    return false;
  }

  try {
    decode_json_obj(val, *iter);
  } catch (err& e) {
    string s = string(name) + ": ";
    s.append(e.message);
    throw err(s);
  }

  return true;
}

template<class C>
bool JSONDecoder::decode_json(const char *name, C& container, void (*cb)(C&, JSONObj *), JSONObj *obj, bool mandatory)
{
  container.clear();

  JSONObjIter iter = obj->find_first(name);
  if (iter.end()) {
    if (mandatory) {
      string s = "missing mandatory field " + string(name);
      throw err(s);
    }
    return false;
  }

  try {
    decode_json_obj(container, cb, *iter);
  } catch (err& e) {
    string s = string(name) + ": ";
    s.append(e.message);
    throw err(s);
  }

  return true;
}

template<class T>
void JSONDecoder::decode_json(const char *name, T& val, T& default_val, JSONObj *obj)
{
  JSONObjIter iter = obj->find_first(name);
  if (iter.end()) {
    val = default_val;
    return;
  }

  try {
    decode_json_obj(val, *iter);
  } catch (err& e) {
    val = default_val;
    string s = string(name) + ": ";
    s.append(e.message);
    throw err(s);
  }
}
class StringLike
{
    public:
    list<string> Referer;
    public:
    void decode_json(JSONObj *obj);
};
void StringLike::decode_json(JSONObj *obj)
{
    JSONDecoder::decode_json("aws:Referer", Referer, obj);
}
class StringNotList
{
    public:
    list<string> Referer;
    public:
    void decode_json(JSONObj *obj);
};
void StringNotList::decode_json(JSONObj *obj)
{
    JSONDecoder::decode_json("aws:Referer", Referer, obj);
}
class ConditionsStr
{
    public:
        StringNotList deny;
        StringLike allow;
    public:
        void decode_json(JSONObj *obj);

};
void ConditionsStr::decode_json(JSONObj *obj)
{
    JSONDecoder::decode_json("StringLike", allow, obj);
    JSONDecoder::decode_json("StringNotLike", deny, obj);
}
class StatementList
{   
    public:
        string Sid;
        string Effect;
        string Principal;
        string Action;
        string Resource;
        ConditionsStr Conditions;
    public:
        void decode_json(JSONObj *obj);
        ConditionsStr& get_conditions() {return Conditions;}
};
void StatementList::decode_json(JSONObj *obj)
{
    JSONDecoder::decode_json("Sid", Sid, obj);
    JSONDecoder::decode_json("Effect", Effect, obj);
    JSONDecoder::decode_json("Principal", Principal, obj);
    JSONDecoder::decode_json("Action", Action, obj);
    JSONDecoder::decode_json("Resource", Resource, obj);
    JSONDecoder::decode_json("Conditions", Conditions, obj);

}
class Policy
{
    public:
        Policy() {}
        ~Policy() {}
        string Version;
        string Id;
        list<StatementList> Statement;
    public:
        void decode_json(JSONObj *obj);
        string& get_version() { return Version; }
        string& get_id() { return Id; }
        list<StatementList> get_statement() { return Statement; }
};
void Policy::decode_json(JSONObj *obj)
{
    JSONDecoder::decode_json("Version", Version, obj);
    JSONDecoder::decode_json("Id", Id, obj);
    JSONDecoder::decode_json("Statement", Statement, obj);
}

int main()
{
    const char* file_name = "/home/caisan/mygithub/Warpper_JSON/policy.json";
    JSONParser parser;
    parser.parse(file_name);
 //JSONObj* iter = parser.find_obj("Statement");
    /*
    multimap<string, JSONObj *> children =  iter->get_children();
    multimap<string, JSONObj *>::iterator child_iter;
    for(child_iter=children.begin(); child_iter!=children.end();++child_iter) {
        cout<<child_iter->second<<endl;
        JSONObj *iter = child_iter->second;
        JSONObj* objiter = iter->find_obj("Sid");
        string sid = objiter->get_data();

        objiter = iter->find_obj("Effect");
        string Effect= objiter->get_data();

        objiter = iter->find_obj("Action");
        string Action= objiter->get_data();
    }
    */
    Policy bucketpolicy;
    decode_json_obj(bucketpolicy, &parser);
    string id  = bucketpolicy.get_id();
    string version  = bucketpolicy.get_version();
    list<StatementList> statement = bucketpolicy.get_statement();
    list<StatementList>::iterator iter;
    for(iter=statement.begin();iter!=statement.end(); ++iter) {
        cout<<iter->Sid<<endl;
        cout<<iter->Effect<<endl;
        ConditionsStr cond = iter->get_conditions();
    }
    return 0;
}
