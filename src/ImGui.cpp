#include <imgui.h>
#include <imgui_stdlib.h>
#include <string>
#include "ImGuiAdapter.hpp"

static void _imgui_payload(imgui_ctx_t* gui)
{
    if (gui->fps != 0)
    {
        gui->now_time = atd_api()->hrtime();
        uint64_t delta = gui->now_time - gui->last_frame;
        gui->last_frame = gui->now_time;

        if (delta < gui->fps_delay)
        {
            atd_api()->sleep((gui->fps_delay - delta) / 1000 / 1000);
        }
    }

    gui->nfy_gui_update->send(gui->nfy_gui_update);
    gui->sem->wait(gui->sem);
}

static void _imgui_thread(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    gui->last_frame = atd_api()->hrtime();

    gui->looping = 1;
    ImGuiAdapter(gui, _imgui_payload);
    gui->looping = 0;

    gui->nfy_gui_update->send(gui->nfy_gui_update);
}

static int _on_gui_loop_beg(lua_State* L, int status, lua_KContext ctx);

static int _on_gui_loop_end(lua_State* L, int status, lua_KContext ctx)
{
    if (status != LUA_OK && status != LUA_YIELD)
    {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        return lua_error(L);
    }

    imgui_ctx_t* gui = (imgui_ctx_t*)ctx;

    /* Notify that GUI loop is done */
    gui->sem->post(gui->sem);

    /* Wait for GUI thread to wakeup */
    gui->co->set_schedule_state(gui->co, LUA_YIELD);
    return lua_yieldk(L, 0, (lua_KContext) gui, _on_gui_loop_beg);
}

static int _on_gui_loop_beg(lua_State* L, int status, lua_KContext ctx)
{
    (void)status;
    imgui_ctx_t* gui = (imgui_ctx_t*)ctx;

    if (!gui->looping)
    {
        return 0;
    }

    int sp = lua_gettop(L);
    assert(sp >= 2);

    lua_pushvalue(L, 2);
    lua_pushvalue(L, 1);

    return _on_gui_loop_end(L,
        lua_pcallk(L, 1, 0, 0, (lua_KContext) gui, _on_gui_loop_end),
        (lua_KContext) gui);
}

static void _on_gui_update(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    /* Wakeup gui coroutine */
    gui->co->set_schedule_state(gui->co, LUA_TNONE);
}

static int _imgui_coroutine(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_touserdata(L, 1);
    gui->co = atd_api()->find_coroutine(L);

    /* Run gui in standalone thread */
    gui->gui_thr = atd_api()->new_thread(_imgui_thread, gui);

    /* Wait for GUI thread to wakeup */
    gui->co->set_schedule_state(gui->co, LUA_YIELD);
    return lua_yieldk(L, 0, (lua_KContext) gui, _on_gui_loop_beg);
}

static int _imgui_gc(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_touserdata(L, 1);
    if (gui->gui_thr != NULL)
    {
        gui->gui_thr->join(gui->gui_thr);
        gui->gui_thr = NULL;
    }
    if (gui->sem != NULL)
    {
        gui->sem->destroy(gui->sem);
        gui->sem = NULL;
    }
    if (gui->nfy_gui_update != NULL)
    {
        gui->nfy_gui_update->destroy(gui->nfy_gui_update);
        gui->nfy_gui_update = NULL;
    }
    if (gui->window_title != NULL)
    {
        free(gui->window_title);
        gui->window_title = NULL;
    }
    return 0;
}

static int _imgui_begin(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);

    bool ret = ImGui::Begin(str);
    lua_pushboolean(L, ret);

    return 1;
}

static int _imgui_end(lua_State *L)
{
    (void)L;
    ImGui::End();
    return 0;
}

static int _imgui_checkbox(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);
    bool is_checked = lua_toboolean(L, 3);

    ImGui::Checkbox(str, &is_checked);

    lua_pushboolean(L, is_checked);
    return 1;
}

static int _imgui_button(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);

    bool ret = ImGui::Button(str);

    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);
    ImGui::Text("%s", str);
    return 0;
}

static int _imgui_same_line(lua_State *L)
{
    (void)L;
    ImGui::SameLine();
    return 0;
}

static int _imgui_bullet_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);
    ImGui::BulletText("%s", str);
    return 0;
}

static int _imgui_input_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);

    std::string data;
    ImGui::InputText(str, &data);

    if (data.size() == 0)
    {
        return 0;
    }

    lua_pushlstring(L, data.c_str(), data.size());
    return 1;
}

static int _imgui_begin_menu_bar(lua_State *L)
{
    bool ret = ImGui::BeginMenuBar();
    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_end_menu_bar(lua_State *L)
{
    (void)L;
    ImGui::EndMenuBar();
    return 0;
}

static int _imgui_begin_menu(lua_State *L)
{
    const char* str = luaL_checkstring(L, 2);
    bool ret = ImGui::BeginMenu(str);
    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_end_menu(lua_State *L)
{
    (void)L;
    ImGui::EndMenu();
    return 0;
}

static int _imgui_menu_item(lua_State *L)
{
    const char* label = luaL_checkstring(L, 2);
    const char* short_cut = lua_tostring(L, 3);

    bool ret = ImGui::MenuItem(label, short_cut);
    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_text_colored(lua_State *L)
{
    float c1 = lua_tonumber(L, 2);
    float c2 = lua_tonumber(L, 3);
    float c3 = lua_tonumber(L, 4);
    float c4 = lua_tonumber(L, 5);
    const char* text = luaL_checkstring(L, 6);

    ImGui::TextColored(ImVec4(c1, c2, c3, c4), "%s", text);
    return 0;
}

static int _imgui_begin_child(lua_State *L)
{
    const char* text = lua_tostring(L, 2);
    ImGui::BeginChild(text);
    return 0;
}

static int _imgui_end_child(lua_State *L)
{
    (void)L;
    ImGui::EndChild();
    return 0;
}

static int _imgui_slider_float(lua_State *L)
{
    const char* label = lua_tostring(L, 2);
    float f = lua_tonumber(L, 3);
    float min = lua_tonumber(L, 4);
    float max = lua_tonumber(L, 5);

    ImGui::SliderFloat(label, &f, min, max);

    lua_pushnumber(L, f);
    return 1;
}

static int _imgui_loop(lua_State *L)
{
    /* auto.coroutine */
    lua_getglobal(L, "auto");
    lua_getfield(L, -1, "coroutine");

    /* arg1: coroutine */
    lua_pushcfunction(L, _imgui_coroutine);

    /* arg2: gui */
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_newuserdata(L, sizeof(imgui_ctx_t));
    memset(gui, 0, sizeof(*gui));
    gui->sem = atd_api()->new_sem(0);
    gui->nfy_gui_update = atd_api()->new_async(_on_gui_update, gui);
    gui->looping = 1;
    gui->fps = 30;
    gui->fps_delay = gui->fps ? (1000.0 / gui->fps) * 1000 * 1000 : 0;
    gui->window_title = strdup("ImGui");

    static const luaL_Reg s_gui_meta[] = {
        { "__gc",       _imgui_gc },
        { NULL,         NULL },
    };
    static const luaL_Reg s_gui_method[] = {
        { "Begin",          _imgui_begin },
        { "BeginChild",     _imgui_begin_child },
        { "BeginMenu",      _imgui_begin_menu },
        { "BeginMenuBar",   _imgui_begin_menu_bar },
        { "BulletText",     _imgui_bullet_text },
        { "Button",         _imgui_button },
        { "CheckBox",       _imgui_checkbox },
        { "End",            _imgui_end },
        { "EndChild",       _imgui_end_child },
        { "EndMenu",        _imgui_end_menu },
        { "EndMenuBar",     _imgui_end_menu_bar },
        { "InputText",      _imgui_input_text },
        { "MenuItem",       _imgui_menu_item },
        { "SameLine",       _imgui_same_line },
        { "SliderFloat",    _imgui_slider_float },
        { "Text",           _imgui_text },
        { "TextColored",    _imgui_text_colored },
        { NULL,             NULL },
    };
    if (luaL_newmetatable(L, "__atd_imgui") != 0)
    {
        luaL_setfuncs(L, s_gui_meta, 0);
        luaL_newlib(L, s_gui_method);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);

    /* arg3: user function */
    lua_pushvalue(L, 1);

    lua_call(L, 3, 1);
    return 1;
}

extern "C"
int luaopen_imgui(lua_State *L)
{
    luaL_checkversion(L);

    static const luaL_Reg s_imgui_method[] = {
        { "loop",   _imgui_loop },
        { NULL,     NULL },
    };

    luaL_newlib(L, s_imgui_method);
    return 1;
}
