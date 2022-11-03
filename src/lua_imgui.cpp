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
    api->lua->pushinteger(L, value);
    api->lua->setfield(L, idx, field);
}

static void _imgui_payload(imgui_ctx_t* gui)
{
    if (gui->fps != 0)
    {
        gui->now_time = api->misc->hrtime();
        uint64_t delta = gui->now_time - gui->last_frame;
        gui->last_frame = gui->now_time;

        if (delta < gui->fps_delay)
        {
            api->thread->sleep((gui->fps_delay - delta) / 1000 / 1000);
        }
    }

    api->notify->send(gui->nfy_gui_update);
    api->sem->wait(gui->sem);
}

static void _imgui_thread(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    gui->last_frame = api->misc->hrtime();

    ImGuiAdapter(gui, _imgui_payload);
    gui->looping = 0;

    api->notify->send(gui->nfy_gui_update);
}

static int _on_gui_loop_beg(lua_State* L, int status, void* ctx);

static int _on_gui_loop_end(lua_State* L, int status, void* ctx)
{
    (void)status;
    imgui_ctx_t* gui = (imgui_ctx_t*)ctx;

    /* Notify that GUI loop is done */
    api->sem->post(gui->sem);

    /* Wait for GUI thread to wakeup */
    api->coroutine->set_state(gui->co, AUTO_COROUTINE_WAIT);
    return api->lua->yieldk(L, 0, gui, _on_gui_loop_beg);
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
static int _on_gui_loop_beg(lua_State* L, int status, void* ctx)
{
    (void)status;
    imgui_ctx_t* gui = (imgui_ctx_t*)ctx;
    int sp = api->lua->gettop(L);

    if (!gui->looping)
    {
        return 0;
    }

    api->lua->pushvalue(L, 2);
    for (int i = 3; i <= sp; i++)
    {
        api->lua->pushvalue(L, i);
    }

    return api->lua->A_callk(L, sp - 2, 0, gui, _on_gui_loop_end);
}

static void _on_gui_update(void* arg)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)arg;

    /* Wakeup gui coroutine */
    api->coroutine->set_state(gui->co, AUTO_COROUTINE_BUSY);
}

static int _imgui_coroutine(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)api->lua->touserdata(L, 1);
    gui->co = api->coroutine->find(L);

    /* Run gui in standalone thread */
    gui->gui_thr = api->thread->create(_imgui_thread, gui);

    /* Wait for GUI thread to wakeup */
    api->coroutine->set_state(gui->co, AUTO_COROUTINE_WAIT);
    return api->lua->yieldk(L, 0, gui, _on_gui_loop_beg);
}

static int _imgui_gc(lua_State *L)
{
    imgui_ctx_t* gui = (imgui_ctx_t*)api->lua->touserdata(L, 1);

    /* Stop gui thread */
    gui->looping = 0;
    api->sem->post(gui->sem);

    if (gui->gui_thr != NULL)
    {
        api->thread->join(gui->gui_thr);
        gui->gui_thr = NULL;
    }
    if (gui->sem != NULL)
    {
        api->sem->destroy(gui->sem);
        gui->sem = NULL;
    }
    if (gui->nfy_gui_update != NULL)
    {
        api->notify->destroy(gui->nfy_gui_update);
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
    const char* str = api->lua->L_checkstring(L, 1);

    /* arg2 */
    bool need_close_icon = true;
    bool is_open = true;
    if (api->lua->type(L, 2) == AUTO_LUA_TBOOLEAN)
    {
        need_close_icon = api->lua->toboolean(L, 2);
    }
    bool* p_open = need_close_icon ? &is_open : NULL;

    /* arg3 */
    ImGuiWindowFlags flag = api->lua->tointeger(L, 3);

    /* call */
    bool ret = ImGui::Begin(str, p_open, flag);
    api->lua->pushboolean(L, ret);
    api->lua->pushboolean(L, is_open);
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
    const char* str = api->lua->L_checkstring(L, 1);
    bool is_checked = api->lua->toboolean(L, 2);

    ImGui::Checkbox(str, &is_checked);

    api->lua->pushboolean(L, is_checked);
    return 1;
}

static int _imgui_button(lua_State *L)
{
    const char* str = api->lua->L_checkstring(L, 1);

    bool ret = ImGui::Button(str);

    api->lua->pushboolean(L, ret);
    return 1;
}

static int _imgui_text(lua_State *L)
{
    const char* str = api->lua->L_checkstring(L, 1);
    ImGui::Text("%s", str);
    return 0;
}

static int _imgui_same_line(lua_State *L)
{
    float offset_from_start_x=0.0f;
    float spacing=-1.0f;

    if (api->lua->type(L, 1) == AUTO_LUA_TNUMBER)
    {
        offset_from_start_x = api->lua->tonumber(L, 1);
    }
    if (api->lua->type(L, 2) == AUTO_LUA_TNUMBER)
    {
        spacing = api->lua->tonumber(L, 2);
    }

    ImGui::SameLine(offset_from_start_x, spacing);
    return 0;
}

static int _imgui_bullet_text(lua_State *L)
{
    const char* str = api->lua->L_checkstring(L, 1);
    ImGui::BulletText("%s", str);
    return 0;
}

static int _imgui_input_text(lua_State *L)
{
    const char* str = api->lua->L_checkstring(L, 1);

    std::string data;
    ImGui::InputText(str, &data);

    if (data.size() == 0)
    {
        return 0;
    }

    api->lua->pushlstring(L, data.c_str(), data.size());
    return 1;
}

static int _imgui_begin_menu_bar(lua_State *L)
{
    bool ret = ImGui::BeginMenuBar();
    api->lua->pushboolean(L, ret);
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
    const char* str = api->lua->L_checkstring(L, 1);
    bool ret = ImGui::BeginMenu(str);
    api->lua->pushboolean(L, ret);
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
    const char* label = api->lua->L_checkstring(L, 1);
    const char* short_cut = api->lua->tostring(L, 2);

    bool ret = ImGui::MenuItem(label, short_cut);
    api->lua->pushboolean(L, ret);
    return 1;
}

static int _imgui_text_colored(lua_State *L)
{
    float c1 = api->lua->L_checknumber(L, 1);
    float c2 = api->lua->L_checknumber(L, 2);
    float c3 = api->lua->L_checknumber(L, 3);
    float c4 = api->lua->L_checknumber(L, 4);
    const char* text = api->lua->L_checkstring(L, 5);

    ImGui::TextColored(ImVec4(c1, c2, c3, c4), "%s", text);
    return 0;
}

static int _imgui_begin_child(lua_State *L)
{
    const char* text = api->lua->L_checkstring(L, 1);
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
    const char* label = api->lua->L_checkstring(L, 1);
    float f = api->lua->L_checknumber(L, 2);
    float min = api->lua->L_checknumber(L, 3);
    float max = api->lua->L_checknumber(L, 4);

    ImGui::SliderFloat(label, &f, min, max);

    api->lua->pushnumber(L, f);
    return 1;
}

static int _imgui_plot_lines(lua_State *L)
{
    const char* label = api->lua->L_checkstring(L, 1);

    if (api->lua->type(L, 2) == AUTO_LUA_TNUMBER)
    {
        int sp = api->lua->gettop(L);
        size_t val_num = sp - 1;
        float* val_array = (float*)malloc(sizeof(float) * val_num);

        for (size_t i = 0; i < val_num; i++)
        {
            val_array[i] = api->lua->tonumber(L, i + 2);
        }

        ImGui::PlotLines(label, val_array, val_num);
        free(val_array);
    }
    else if (api->lua->type(L, 2) == AUTO_LUA_TTABLE)
    {
        size_t val_num = (size_t)api->lua->L_len(L, 2);
        float* val_array = (float*)malloc(sizeof(float) * val_num);

        api->lua->pushnil(L);
        for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
        {
            val_array[i] = api->lua->tonumber(L, -1);
            api->lua->pop(L, 1);
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
    api->lua->pushboolean(L, show);
    return 1;
}

static int _imgui_show_metrics_window(lua_State *L)
{
    bool show = true;
    ImGui::ShowMetricsWindow(&show);
    api->lua->pushboolean(L, show);
    return 1;
}

static int _imgui_show_stack_tool_window(lua_State *L)
{
    bool show = true;
    ImGui::ShowStackToolWindow(&show);
    api->lua->pushboolean(L, show);
    return 1;
}

static int _imgui_get_window_pos(lua_State *L)
{
    ImVec2 ret = ImGui::GetWindowPos();
    api->lua->pushnumber(L, ret.x);
    api->lua->pushnumber(L, ret.y);
    return 2;
}

static int _imgui_get_window_size(lua_State *L)
{
    ImVec2 ret = ImGui::GetWindowSize();
    api->lua->pushnumber(L, ret.x);
    api->lua->pushnumber(L, ret.y);
    return 2;
}

static int _imgui_set_next_window_pos(lua_State *L)
{
    float c1 = api->lua->tonumber(L, 1);
    float c2 = api->lua->tonumber(L, 2);
    ImVec2 pos(c1, c2);
    ImGui::SetNextWindowPos(pos);
    return 0;
}

static int _imgui_set_next_window_size(lua_State *L)
{
    float c1 = api->lua->tonumber(L, 1);
    float c2 = api->lua->tonumber(L, 2);
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
    float c1 = api->lua->tonumber(L, 1);
    float c2 = api->lua->tonumber(L, 2);
    ImVec2 size(c1, c2);
    ImGui::Dummy(size);
    return 0;
}

static int _imgui_indent(lua_State *L)
{
    float indent_w = 0.0f;
    if (api->lua->type(L, 1) == AUTO_LUA_TNUMBER)
    {
        indent_w = api->lua->tonumber(L, 1);
    }
    ImGui::Indent(indent_w);
    return 0;
}

static int _imgui_unindent(lua_State *L)
{
    float indent_w = 0.0f;
    if (api->lua->type(L, 1) == AUTO_LUA_TNUMBER)
    {
        indent_w = api->lua->tonumber(L, 1);
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
    api->lua->pushnumber(L, ret.x);
    api->lua->pushnumber(L, ret.y);
    return 2;
}

static int _imgui_set_cursor_pos(lua_State *L)
{
    float c1 = api->lua->tonumber(L, 1);
    float c2 = api->lua->tonumber(L, 2);
    ImVec2 pos(c1, c2);
    ImGui::SetCursorPos(pos);
    return 0;
}

static int _imgui_get_cursor_screen_pos(lua_State *L)
{
    ImVec2 ret = ImGui::GetCursorScreenPos();
    api->lua->pushnumber(L, ret.x);
    api->lua->pushnumber(L, ret.y);
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
    api->lua->pushnumber(L, ret);
    return 1;
}

static int _imgui_get_frame_height(lua_State *L)
{
    float ret = ImGui::GetFrameHeight();
    api->lua->pushnumber(L, ret);
    return 1;
}

static int _imgui_options(lua_State* L, int idx, imgui_ctx_t* gui)
{
    if (api->lua->getfield(L, idx, "window_size") == AUTO_LUA_TSTRING)
    {
        sscanf(api->lua->tostring(L, -1), "%dx%d", &gui->window.x, &gui->window.y);
    }
    api->lua->pop(L, 1);

    if (api->lua->getfield(L, idx, "window_title") == AUTO_LUA_TSTRING)
    {
        if (gui->window.title != NULL)
        {
            free(gui->window.title);
        }
        gui->window.title = strdup(api->lua->tostring(L, -1));
    }
    api->lua->pop(L, 1);

    return 0;
}

static void _imgui_initialize_to_default(lua_State* L, imgui_ctx_t* gui)
{
    gui->sem = api->sem->create(0);
    gui->nfy_gui_update = api->notify->create(L, _on_gui_update, gui);
    gui->looping = 1;
    gui->fps = 30;
    gui->fps_delay = gui->fps ? (1000.0 / gui->fps) * 1000 * 1000 : 0;
    gui->window.title = strdup("ImGui");
    gui->window.x = 1280;
    gui->window.y = 720;
}

static int _imgui_loop_after(struct lua_State* L, int status, void* ctx)
{
    (void)L; (void)status; (void)ctx;
    return 1;
}

static int _imgui_loop(lua_State* L)
{
    int sp = api->lua->gettop(L);

    /* auto.coroutine */
    api->lua->getglobal(L, "auto");
    api->lua->getfield(L, -1, "coroutine");

    /* arg1: coroutine */
    api->lua->pushcfunction(L, _imgui_coroutine);

    /* arg2: gui */
    imgui_ctx_t* gui = (imgui_ctx_t*)api->lua->newuserdatauv(L, sizeof(imgui_ctx_t), 0);
    memset(gui, 0, sizeof(*gui));
    _imgui_initialize_to_default(L, gui);
    _imgui_options(L, 1, gui);

    static const auto_luaL_Reg s_gui_meta[] = {
        { "__gc",       _imgui_gc },
        { NULL,         NULL },
    };
    if (api->lua->L_newmetatable(L, "__atd_imgui") != 0)
    {
        api->lua->L_setfuncs(L, s_gui_meta, 0);
    }
    api->lua->setmetatable(L, -2);

    /* arg3+: user function and arguments */
    for (int i = 2; i <= sp; i++)
    {
        api->lua->pushvalue(L, i);
    }

    return api->lua->A_callk(L, sp + 1, 1, NULL, _imgui_loop_after);
}

static int _luaopen_imgui(lua_State *L)
{
    static const auto_luaL_Reg s_imgui_method[] = {
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
    api->lua->L_newlib(L, s_imgui_method);

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
    /* Get API */
    api = auto_api();

    _luaopen_imgui(L);

    imgui_luaopen_implot(L);
    api->lua->setfield(L, -2, "implot");

    return 1;
}
