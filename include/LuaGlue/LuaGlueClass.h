#ifndef LUAGLUE_CLASS_H_GUARD
#define LUAGLUE_CLASS_H_GUARD

#include <vector>
#include <string>
#include <map>

#include "LuaGlue/LuaGlueClassBase.h"
#include "LuaGlue/LuaGlueConstant.h"
#include "LuaGlue/LuaGlueMethodBase.h"

class LuaGlue;

template<typename _Class, typename... _Args>
class LuaGlueCtorMethod;

template<typename _Class>
class LuaGlueDtorMethod;

template<typename _Ret, typename _Class, typename... _Args>
class LuaGlueStaticMethod;

template<typename _Class, typename... _Args>
class LuaGlueStaticMethod<void, _Class, _Args...>;

template<typename _Ret, typename _Class, typename... _Args>
class LuaGlueMethod;

template<typename _Class, typename... _Args>
class LuaGlueMethod<void, _Class, _Args...>;

template<typename _Value, typename _Class, typename _Key>
class LuaGlueIndexMethod;

template<typename _Value, typename _Class, typename _Key>
class LuaGlueNewIndexMethod;

// TODO: look into associating classes and methods with an index into
//  a lookup table rather than with a lightuserdata to the class itself..
// maybe an unordered_map of typeid(TC).hash_code() for classes ?
template<typename _Class>
class LuaGlueClass : public LuaGlueClassBase
{
	public:
		typedef _Class ClassType;
		
		LuaGlueClass(LuaGlue *luaGlue, const std::string &name) : luaGlue_(luaGlue), name_(name)
		{ }
		
		~LuaGlueClass() { }
		
		const std::string &name() { return name_; }
		
		template<typename _Ret, typename... _Args>
		_Ret invokeLuaMethod(const std::string &name, _Class *obj, _Args... args)
		{
			// TODO: maybe add LuaGlueObject wrapper, so we can create a single userdata
			//  and just pass it around.
		}
		
		LuaGlueClass<_Class> &pushInstance(_Class *)
		{
			/*_Class **udata = (_Class **)lua_newuserdata(state, sizeof(_Class *));
			*udata = obj;
			
			luaL_getmetatable(state, glueClass->name().c_str());
			lua_setmetatable(state, -2);
			*/
			return *this;
		}
		
		template<typename... _Args>
		LuaGlueClass<_Class> &ctor(const std::string &name)
		{
			auto impl = new LuaGlueCtorMethod<_Class, _Args...>(this, name.c_str());
			static_methods[name] = impl;
			
			return *this;
		}
		
		LuaGlueClass<_Class> &dtor(void (_Class::*fn)())
		{
			auto impl = new LuaGlueDtorMethod<_Class>(this, "__gc", std::forward<decltype(fn)>(fn));
			meta_methods["__gc"] = impl;
			
			return *this;
		}
		
		template<typename _Value, typename _Key>
		LuaGlueClass<_Class> &index(_Value (_Class::*fn)(_Key))
		{
			auto impl = new LuaGlueIndexMethod<_Value, _Class, _Key>(this, "__index", std::forward<decltype(fn)>(fn));
			meta_methods["__index"] = impl;
			
			return *this;
		}
		
		template<typename _Value, typename _Key>
		LuaGlueClass<_Class> &newindex(void (_Class::*fn)(_Key, _Value))
		{
			auto impl = new LuaGlueNewIndexMethod<_Value, _Class, _Key>(this, "__newindex", std::forward<decltype(fn)>(fn));
			meta_methods["__newindex"] = impl;
			
			return *this;
		}
		
		template<typename _Ret, typename... _Args>
		LuaGlueClass<_Class> &method(const std::string &name, _Ret (_Class::*fn)(_Args...))
		{
			auto impl = new LuaGlueMethod<_Ret, _Class, _Args...>(this, name, std::forward<decltype(fn)>(fn));
			methods[name] = impl;
			
			return *this;
		}
		
		template<typename... _Args>
		LuaGlueClass<_Class> &method(const std::string &name, void (_Class::*fn)(_Args...))
		{
			auto impl = new LuaGlueMethod<void, _Class, _Args...>(this, name, std::forward<decltype(fn)>(fn));
			methods[name] = impl;
			
			return *this;
		}
		
		template<typename _Ret, typename... _Args>
		LuaGlueClass<_Class> &method(const std::string &name, _Ret (*fn)(_Args...))
		{
			auto impl = new LuaGlueStaticMethod<_Ret, _Class, _Args...>(this, name, std::forward<decltype(fn)>(fn));
			static_methods[name] = impl;
			
			return *this;
		}
		
		template<typename... _Args>
		LuaGlueClass<_Class> &method(const std::string &name, void (*fn)(_Args...))
		{
			auto impl = new LuaGlueStaticMethod<void, _Class, _Args...>(this, name, std::forward<decltype(fn)>(fn));
			static_methods[name] = impl;
			
			return *this;
		}
		
		template<typename T>
		LuaGlueClass<_Class> &constant(const std::string &name, T v)
		{
			auto impl = new LuaGlueConstant(name, v);
			constants[name] = impl;
			
			return *this;
		}
		
		LuaGlueClass<_Class> &constants(const std::vector<LuaGlueConstant> &c)
		{
			for(unsigned int i = 0; i < c.size(); i++)
			{
				auto impl = new LuaGlueConstant(c[i]);
				constants_[impl->name()] = impl;
			}
			
			return *this;
		}
		
		LuaGlue &end() { return *luaGlue_; }
		LuaGlue &luaGlue() { return *luaGlue_; }
		
		bool glue(LuaGlue *luaGlue)
		{
			lua_createtable(luaGlue->state(), 0, 0);
			int lib_id = lua_gettop(luaGlue->state());
			
			//printf("Glue Class: %s\n", name_.c_str());
			luaL_newmetatable(luaGlue->state(), name_.c_str());
			int meta_id = lua_gettop(luaGlue->state());
			
			lua_pushvalue(luaGlue->state(), -1);
			lua_setglobal(luaGlue->state(), name_.c_str());
			
			
			
			lua_pushvalue(luaGlue->state(), lib_id);
			for(auto &method: meta_methods)
			{
				//printf("Glue method: %s::%s\n", name_.c_str(), method.first.c_str());
				if(!method.second->glue(luaGlue))
					return false;
			}
			lua_setfield(luaGlue->state(), meta_id, "__metatable");
			
			lua_pushvalue(luaGlue->state(), lib_id);
			for(auto &method: methods)
			{
				//printf("Glue method: %s::%s\n", name_.c_str(), method.first.c_str());
				if(!method.second->glue(luaGlue))
					return false;
			}
			lua_setfield(luaGlue->state(), meta_id, "__index");
			
			lua_createtable(luaGlue->state(), 0, 0);
			
			for(auto &method: static_methods)
			{
				//printf("Glue static method: %s::%s\n", name_.c_str(), method.first.c_str());
				if(!method.second->glue(luaGlue))
					return false;
			}
			
			for(auto &constant: constants_)
			{
				//printf("Glue constant: %s::%s\n", name_.c_str(), constant.first.c_str());
				if(!constant.second->glue(luaGlue))
					return false;
			}
			
			lua_setmetatable(luaGlue->state(), lib_id);
			
			lua_pop(luaGlue->state(), 1);
			//lua_stack_dump(luaGlue->state());
			//printf("done.\n");
			return true;
		}
		
	private:
		LuaGlue *luaGlue_;
		std::string name_;
		
		std::map<std::string, LuaGlueConstant *> constants_;
		std::map<std::string, LuaGlueMethodBase *> methods;
		std::map<std::string, LuaGlueMethodBase *> static_methods;
		std::map<std::string, LuaGlueMethodBase *> meta_methods;
		
		void lua_gc()
		{
			//printf("%s::__gc!\n", name_.c_str());
		}
};

#endif /* LUAGLUE_CLASS_H_GUARD */
