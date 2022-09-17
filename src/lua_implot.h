#ifndef __LUA_IMPLOT_H__
#define __LUA_IMPLOT_H__

#include <autodo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open ImGui extension implot.
 * @param[in] L     Lua VM.
 * @return          Always 1.
 */
AUTO_LOCAL int imgui_luaopen_implot(lua_State *L);

#ifdef __cplusplus
}
#endif
#endif
