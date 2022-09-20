#include <imgui.h>
#include <imgui_stdlib.h>
#include <string>
#include "ImGuiAdapter.hpp"
#include "lua_implot.h"
#include "lua_imgui.h"

#define LUA_IMGUI_SET_FLAG(x)   \
    _imgui_add_constant(L, -2, #x, x)

const auto_api_t* api;

static void _imgui_add_constant(lua_State* L, int idx, const char* field, int64_t value)
{
    api->int64.push_value(L, value);
    lua_setfield(L, idx, field);
}

static void _imgui_payload(imgui_ctx_t* gui)
{
    if (gui->fps != 0)
    {
        gui->now_time = api->misc.hrtime();
        uint64_t delta = gui->now_time - gui->last_frame;
        gui->last_frame = gui->now_time;

        if (delta < gui->fps_delay)
        {
            api->thread.sleep((gui->fps_delay - delta) / 1000 / 1000);
        }
    }

    api->async.send(gui->nfy_gui_update);
    api->sem.wait(gui->sem);
}

static void _imgui_thread(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    gui->last_frame = api->misc.hrtime();

    gui->looping = 1;
    ImGuiAdapter(gui, _imgui_payload);
    gui->looping = 0;

    api->async.send(gui->nfy_gui_update);
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
    api->sem.post(gui->sem);

    /* Wait for GUI thread to wakeup */
    api->coroutine.set_state(gui->co, LUA_YIELD);
    return lua_yieldk(L, 0, (lua_KContext) gui, _on_gui_loop_beg);
}

/**
 * @brief Gui loop.
 *
 * The initialize stack layout:
 * [1]: #imgui_ctx_t
 * [2]: user function
 * [3+]: user arguments
 *
 * @param L
 * @param status
 * @param ctx
 * @return
 */
static int _on_gui_loop_beg(lua_State* L, int status, lua_KContext ctx)
{
    (void)status;
    imgui_ctx_t* gui = (imgui_ctx_t*)ctx;
    int sp = lua_gettop(L);

    if (!gui->looping)
    {
        return 0;
    }

    lua_pushvalue(L, 2);
    for (int i = 3; i <= sp; i++)
    {
        lua_pushvalue(L, i);
    }

    return _on_gui_loop_end(L,
        lua_pcallk(L, sp - 2, 0, 0, (lua_KContext) gui, _on_gui_loop_end),
        (lua_KContext) gui);
}

static void _on_gui_update(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    /* Wakeup gui coroutine */
    api->coroutine.set_state(gui->co, LUA_TNONE);
}

static int _imgui_coroutine(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_touserdata(L, 1);
    gui->co = api->coroutine.find(L);

    /* Run gui in standalone thread */
    gui->gui_thr = api->thread.create(_imgui_thread, gui);

    /* Wait for GUI thread to wakeup */
    api->coroutine.set_state(gui->co, LUA_YIELD);
    return lua_yieldk(L, 0, (lua_KContext) gui, _on_gui_loop_beg);
}

static int _imgui_gc(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_touserdata(L, 1);
    if (gui->gui_thr != NULL)
    {
        api->thread.join(gui->gui_thr);
        gui->gui_thr = NULL;
    }
    if (gui->sem != NULL)
    {
        api->sem.destroy(gui->sem);
        gui->sem = NULL;
    }
    if (gui->nfy_gui_update != NULL)
    {
        api->async.destroy(gui->nfy_gui_update);
        gui->nfy_gui_update = NULL;
    }
    if (gui->window.title != NULL)
    {
        free(gui->window.title);
        gui->window.title = NULL;
    }
    return 0;
}

static int _imgui_begin(lua_State *L)
{
    /* arg1 */
    const char* str = luaL_checkstring(L, 1);

    /* arg2 */
    bool need_close_icon = true;
    bool is_open = true;
    if (lua_isboolean(L, 2))
    {
        need_close_icon = lua_toboolean(L, 2);
    }
    bool* p_open = need_close_icon ? &is_open : NULL;

    /* arg3 */
    int64_t l_flag;
    ImGuiWindowFlags flag = api->int64.get_value(L, 3, &l_flag) ? (ImGuiWindowFlags)l_flag : ImGuiWindowFlags_None;

    /* call */
    bool ret = ImGui::Begin(str, p_open, flag);
    lua_pushboolean(L, ret);
    lua_pushboolean(L, is_open);
    return 2;
}

static int _imgui_end(lua_State *L)
{
    (void)L;
    ImGui::End();
    return 0;
}

static int _imgui_checkbox(lua_State *L)
{
    const char* str = luaL_checkstring(L, 1);
    bool is_checked = lua_toboolean(L, 2);

    ImGui::Checkbox(str, &is_checked);

    lua_pushboolean(L, is_checked);
    return 1;
}

static int _imgui_button(lua_State *L)
{
    const char* str = luaL_checkstring(L, 1);

    bool ret = ImGui::Button(str);

    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 1);
    ImGui::Text("%s", str);
    return 0;
}

static int _imgui_same_line(lua_State *L)
{
    float offset_from_start_x=0.0f;
    float spacing=-1.0f;

    if (lua_isnumber(L, 1))
    {
        offset_from_start_x = lua_tonumber(L, 1);
    }
    if (lua_isnumber(L, 2))
    {
        spacing = lua_tonumber(L, 2);
    }

    ImGui::SameLine(offset_from_start_x, spacing);
    return 0;
}

static int _imgui_bullet_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 1);
    ImGui::BulletText("%s", str);
    return 0;
}

static int _imgui_input_text(lua_State *L)
{
    const char* str = luaL_checkstring(L, 1);

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
    const char* str = luaL_checkstring(L, 1);
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
    const char* label = luaL_checkstring(L, 1);
    const char* short_cut = lua_tostring(L, 2);

    bool ret = ImGui::MenuItem(label, short_cut);
    lua_pushboolean(L, ret);
    return 1;
}

static int _imgui_text_colored(lua_State *L)
{
    float c1 = luaL_checknumber(L, 1);
    float c2 = luaL_checknumber(L, 2);
    float c3 = luaL_checknumber(L, 3);
    float c4 = luaL_checknumber(L, 4);
    const char* text = luaL_checkstring(L, 5);

    ImGui::TextColored(ImVec4(c1, c2, c3, c4), "%s", text);
    return 0;
}

static int _imgui_begin_child(lua_State *L)
{
    const char* text = luaL_checkstring(L, 1);
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
    const char* label = luaL_checkstring(L, 1);
    float f = luaL_checknumber(L, 2);
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);

    ImGui::SliderFloat(label, &f, min, max);

    lua_pushnumber(L, f);
    return 1;
}

static int _imgui_plot_lines(lua_State *L)
{
    const char* label = luaL_checkstring(L, 1);

    if (lua_isnumber(L, 2))
    {
        int sp = lua_gettop(L);
        size_t val_num = sp - 1;
        float* val_array = (float*)malloc(sizeof(float) * val_num);

        for (size_t i = 0; i < val_num; i++)
        {
            val_array[i] = lua_tonumber(L, i + 2);
        }

        ImGui::PlotLines(label, val_array, val_num);
        free(val_array);
    }
    else if (lua_istable(L, 2))
    {
        size_t val_num = (size_t)luaL_len(L, 2);
        float* val_array = (float*)malloc(sizeof(float) * val_num);

        lua_pushnil(L);
        for (size_t i = 0; lua_next(L, 2) != 0; i++)
        {
            val_array[i] = lua_tonumber(L, -1);
            lua_pop(L, 1);
        }

        ImGui::PlotLines(label, val_array, val_num);
        free(val_array);
    }

    return 0;
}

static int _imgui_show_demo_window(lua_State *L)
{
    bool show = true;
    ImGui::ShowDemoWindow(&show);
    lua_pushboolean(L, show);
    return 1;
}

static int _imgui_show_metrics_window(lua_State *L)
{
    bool show = true;
    ImGui::ShowMetricsWindow(&show);
    lua_pushboolean(L, show);
    return 1;
}

static int _imgui_show_stack_tool_window(lua_State *L)
{
    bool show = true;
    ImGui::ShowStackToolWindow(&show);
    lua_pushboolean(L, show);
    return 1;
}

static int _imgui_get_window_pos(lua_State *L)
{
    ImVec2 ret = ImGui::GetWindowPos();
    lua_pushnumber(L, ret.x);
    lua_pushnumber(L, ret.y);
    return 2;
}

static int _imgui_get_window_size(lua_State *L)
{
    ImVec2 ret = ImGui::GetWindowSize();
    lua_pushnumber(L, ret.x);
    lua_pushnumber(L, ret.y);
    return 2;
}

static int _imgui_set_next_window_pos(lua_State *L)
{
    float c1 = lua_tonumber(L, 1);
    float c2 = lua_tonumber(L, 2);
    ImVec2 pos(c1, c2);
    ImGui::SetNextWindowPos(pos);
    return 0;
}

static int _imgui_set_next_window_size(lua_State *L)
{
    float c1 = lua_tonumber(L, 1);
    float c2 = lua_tonumber(L, 2);
    ImVec2 size(c1, c2);
    ImGui::SetNextWindowSize(size);
    return 0;
}

static int _imgui_set_next_window_focus(lua_State *L)
{
    (void)L;
    ImGui::SetNextWindowFocus();
    return 0;
}

static int _imgui_separator(lua_State *L)
{
    (void)L;
    ImGui::Separator();
    return 0;
}

static int _imgui_new_line(lua_State *L)
{
    (void)L;
    ImGui::NewLine();
    return 0;
}

static int _imgui_spacing(lua_State *L)
{
    (void)L;
    ImGui::Spacing();
    return 0;
}

static int _imgui_dummy(lua_State *L)
{
    float c1 = lua_tonumber(L, 1);
    float c2 = lua_tonumber(L, 2);
    ImVec2 size(c1, c2);
    ImGui::Dummy(size);
    return 0;
}

static int _imgui_indent(lua_State *L)
{
    float indent_w = 0.0f;
    if (lua_isnumber(L, 1))
    {
        indent_w = lua_tonumber(L, 1);
    }
    ImGui::Indent(indent_w);
    return 0;
}

static int _imgui_unindent(lua_State *L)
{
    float indent_w = 0.0f;
    if (lua_isnumber(L, 1))
    {
        indent_w = lua_tonumber(L, 1);
    }
    ImGui::Unindent(indent_w);
    return 0;
}

static int _imgui_begin_group(lua_State *L)
{
    (void)L;
    ImGui::BeginGroup();
    return 0;
}

static int _imgui_end_group(lua_State *L)
{
    (void)L;
    ImGui::EndGroup();
    return 0;
}

static int _imgui_get_cursor_pos(lua_State *L)
{
    ImVec2 ret = ImGui::GetCursorPos();
    lua_pushnumber(L, ret.x);
    lua_pushnumber(L, ret.y);
    return 2;
}

static int _imgui_set_cursor_pos(lua_State *L)
{
    float c1 = lua_tonumber(L, 1);
    float c2 = lua_tonumber(L, 2);
    ImVec2 pos(c1, c2);
    ImGui::SetCursorPos(pos);
    return 0;
}

static int _imgui_get_cursor_screen_pos(lua_State *L)
{
    ImVec2 ret = ImGui::GetCursorScreenPos();
    lua_pushnumber(L, ret.x);
    lua_pushnumber(L, ret.y);
    return 2;
}

static int _imgui_align_text_to_frame_padding(lua_State *L)
{
    (void)L;
    ImGui::AlignTextToFramePadding();
    return 0;
}

static int _imgui_get_text_line_height(lua_State *L)
{
    float ret = ImGui::GetTextLineHeight();
    lua_pushnumber(L, ret);
    return 1;
}

static int _imgui_get_frame_height(lua_State *L)
{
    float ret = ImGui::GetFrameHeight();
    lua_pushnumber(L, ret);
    return 1;
}

static int _imgui_options(lua_State* L, int idx, imgui_ctx_t* gui)
{
    if (lua_getfield(L, idx, "window_size") == LUA_TSTRING)
    {
        sscanf(lua_tostring(L, -1), "%dx%d", &gui->window.x, &gui->window.y);
    }
    lua_pop(L, 1);

    if (lua_getfield(L, idx, "window_title") == LUA_TSTRING)
    {
        if (gui->window.title != NULL)
        {
            free(gui->window.title);
        }
        gui->window.title = strdup(lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    return 0;
}

static void _imgui_initialize_to_default(imgui_ctx_t* gui)
{
    gui->sem = api->sem.create(0);
    gui->nfy_gui_update = api->async.create(_on_gui_update, gui);
    gui->looping = 1;
    gui->fps = 30;
    gui->fps_delay = gui->fps ? (1000.0 / gui->fps) * 1000 * 1000 : 0;
    gui->window.title = strdup("ImGui");
    gui->window.x = 1280;
    gui->window.y = 720;
}

static int _imgui_loop(lua_State *L)
{
    int sp = lua_gettop(L);

    /* auto.coroutine */
    lua_getglobal(L, "auto");
    lua_getfield(L, -1, "coroutine");

    /* arg1: coroutine */
    lua_pushcfunction(L, _imgui_coroutine);

    /* arg2: gui */
    imgui_ctx_t* gui = (imgui_ctx_t*)lua_newuserdata(L, sizeof(imgui_ctx_t));
    memset(gui, 0, sizeof(*gui));
    _imgui_initialize_to_default(gui);
    _imgui_options(L, 1, gui);

    static const luaL_Reg s_gui_meta[] = {
        { "__gc",       _imgui_gc },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(L, "__atd_imgui") != 0)
    {
        luaL_setfuncs(L, s_gui_meta, 0);
    }
    lua_setmetatable(L, -2);

    /* arg3+: user function and arguments */
    for (int i = 2; i <= sp; i++)
    {
        lua_pushvalue(L, i);
    }

    lua_call(L, sp + 1, 1);
    return 1;
}

static int _luaopen_imgui(lua_State *L)
{
    static const luaL_Reg s_imgui_method[] = {
        { "loop",                       _imgui_loop },
        { "AlignTextToFramePadding",    _imgui_align_text_to_frame_padding },
        { "Begin",                      _imgui_begin },
        { "BeginChild",                 _imgui_begin_child },
        { "BeginGroup",                 _imgui_begin_group },
        { "BeginMenu",                  _imgui_begin_menu },
        { "BeginMenuBar",               _imgui_begin_menu_bar },
        { "BulletText",                 _imgui_bullet_text },
        { "Button",                     _imgui_button },
        { "CheckBox",                   _imgui_checkbox },
        { "Dummy",                      _imgui_dummy },
        { "End",                        _imgui_end },
        { "EndChild",                   _imgui_end_child },
        { "EndGroup",                   _imgui_end_group },
        { "EndMenu",                    _imgui_end_menu },
        { "EndMenuBar",                 _imgui_end_menu_bar },
        { "GetCursorPos",               _imgui_get_cursor_pos },
        { "GetCursorScreenPos",         _imgui_get_cursor_screen_pos },
        { "GetFrameHeight",             _imgui_get_frame_height },
        { "GetTextLineHeight",          _imgui_get_text_line_height },
        { "GetWindowPos",               _imgui_get_window_pos },
        { "GetWindowSize",              _imgui_get_window_size },
        { "Indent",                     _imgui_indent },
        { "InputText",                  _imgui_input_text },
        { "NewLine",                    _imgui_new_line },
        { "MenuItem",                   _imgui_menu_item },
        { "PlotLines",                  _imgui_plot_lines },
        { "SameLine",                   _imgui_same_line },
        { "Separator",                  _imgui_separator },
        { "SetCursorPos",               _imgui_set_cursor_pos },
        { "SetNextWindowFocus",         _imgui_set_next_window_focus },
        { "SetNextWindowPos",           _imgui_set_next_window_pos },
        { "SetNextWindowSize",          _imgui_set_next_window_size },
        { "ShowDemoWindow",             _imgui_show_demo_window },
        { "ShowMetricsWindow",          _imgui_show_metrics_window },
        { "ShowStackToolWindow",        _imgui_show_stack_tool_window },
        { "SliderFloat",                _imgui_slider_float },
        { "Spacing",                    _imgui_spacing },
        { "Text",                       _imgui_text },
        { "TextColored",                _imgui_text_colored },
        { "Unindent",                   _imgui_unindent },
        { NULL,                         NULL },
    };
    luaL_newlib(L, s_imgui_method);

    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_None);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoTitleBar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoResize);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoMove);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoScrollbar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoScrollWithMouse);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoCollapse);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_AlwaysAutoResize);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoBackground);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoSavedSettings);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoMouseInputs);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_MenuBar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_HorizontalScrollbar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoFocusOnAppearing);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoBringToFrontOnFocus);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_AlwaysUseWindowPadding);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoNavInputs);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoNavFocus);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_UnsavedDocument);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoNav);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoDecoration);
    LUA_IMGUI_SET_FLAG(ImGuiWindowFlags_NoInputs);

    return 1;
}


int luaopen_imgui(lua_State *L)
{
    luaL_checkversion(L);
    api = AUTO_GET_API();

    _luaopen_imgui(L);

    imgui_luaopen_implot(L);
    lua_setfield(L, -2, "implot");

    return 1;
}
