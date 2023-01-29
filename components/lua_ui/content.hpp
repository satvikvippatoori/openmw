#ifndef COMPONENTS_LUAUI_CONTENT
#define COMPONENTS_LUAUI_CONTENT

#include <map>
#include <string>

#include <sol/sol.hpp>

namespace LuaUi::Content
{
    sol::protected_function makeFactory(sol::state_view);

    class View
    {
    public:
        static int64_t sInstanceCount; // debug information, shown in Lua profiler

        // accepts only Lua tables returned by ui.content
        explicit View(sol::table table)
            : mTable(std::move(table))
        {
            if (!isValid(mTable))
                throw std::domain_error("Expected a Content table");
            sInstanceCount++;
        }
        View(const View& c)
        {
            this->mTable = c.mTable;
            sInstanceCount++;
        }
        View(View&& c)
        {
            this->mTable = std::move(c.mTable);
            sInstanceCount++;
        }
        ~View() { sInstanceCount--; }

        static bool isValid(const sol::object& object)
        {
            if (object.get_type() != sol::type::table)
                return false;
            sol::table table = object;
            return table.traverse_get<sol::optional<bool>>(sol::metatable_key, "__Content").value_or(false);
        }

        size_t size() const { return mTable.size(); }

        void assign(std::string_view name, const sol::table& table)
        {
            if (indexOf(name).has_value())
                mTable[name] = table;
            else
                throw std::domain_error("Invalid Content key");
        }
        void assign(size_t index, const sol::table& table)
        {
            if (index <= size())
                mTable[toLua(index)] = table;
            else
                throw std::domain_error("Invalid Content index");
        }
        void insert(size_t index, const sol::table& table) { callMethod("insert", toLua(index), table); }

        sol::table at(size_t index) const
        {
            if (index < size())
                return mTable.get<sol::table>(toLua(index));
            else
                throw std::domain_error("Invalid Content index");
        }
        sol::table at(std::string_view name) const
        {
            if (indexOf(name).has_value())
                return mTable.get<sol::table>(name);
            else
                throw std::domain_error("Invalid Content key");
        }
        void remove(size_t index)
        {
            if (index < size())
                mTable[toLua(index)] = sol::nil;
            else
                throw std::domain_error("Invalid Content index");
        }
        void remove(std::string_view name)
        {
            if (indexOf(name).has_value())
                mTable[name] = sol::nil;
            else
                throw std::domain_error("Invalid Content index");
        }
        std::optional<size_t> indexOf(std::string_view name) const
        {
            sol::object result = callMethod("indexOf", name);
            if (result.is<size_t>())
                return fromLua(result.as<size_t>());
            else
                return std::nullopt;
        }
        std::optional<size_t> indexOf(const sol::table& table) const
        {
            sol::object result = callMethod("indexOf", table);
            if (result.is<size_t>())
                return fromLua(result.as<size_t>());
            else
                return std::nullopt;
        }

    private:
        sol::table mTable;

        template <typename... Arg>
        sol::object callMethod(std::string_view name, Arg&&... arg) const
        {
            return mTable.get<sol::protected_function>(name)(mTable, arg...);
        }

        static inline size_t toLua(size_t index) { return index + 1; }
        static inline size_t fromLua(size_t index) { return index - 1; }
    };

    int64_t getInstanceCount();
}

#endif // COMPONENTS_LUAUI_CONTENT
