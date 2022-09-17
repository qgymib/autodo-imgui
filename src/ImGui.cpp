#include "lua_imgui.h"
#include "lua_implot.h"

extern "C"
int luaopen_imgui(lua_State *L)
{
    luaL_checkversion(L);

    imgui_luaopen_imgui(L);

    imgui_luaopen_implot(L);
    lua_setfield(L, -2, "implot");

    return 1;
}
