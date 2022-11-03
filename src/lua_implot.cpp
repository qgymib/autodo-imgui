#include "lua_implot.h"
#include "lua_imgui.h"
#include <cstdlib>
#include <implot.h>

static int _implot_begin_plot(lua_State *L)
{
    const char* title_id = api->lua->L_checklstring(L, 1, NULL);
    bool ret = ImPlot::BeginPlot(title_id);
    api->lua->pushboolean(L, ret);
    return 1;
}

static int _implot_end_plot(lua_State *L)
{
    (void)L;
    ImPlot::EndPlot();
    return 0;
}

static int _implot_plot_bars(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotBars(label_id, values, len);
    return 0;
}

static int _implot_plot_line(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotLine(label_id, values, len);
    free(values);

    return 0;
}

static int _implot_plot_scatter(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotScatter(label_id, values, len);
    free(values);

    return 0;
}

static int _implot_plot_stairs(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotStairs(label_id, values, len);
    free(values);

    return 0;
}

static int _implot_plot_shaded(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotShaded(label_id, values, len);
    free(values);

    return 0;
}

static int _implot_plot_stems(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);

    size_t len = api->lua->L_len(L, 2);
    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotStems(label_id, values, len);
    free(values);

    return 0;
}

static int _implot_plot_heatmap(lua_State *L)
{
    const char* label_id = api->lua->L_checklstring(L, 1, NULL);
    api->lua->L_checktype(L, 2, AUTO_LUA_TTABLE);
    int rows = api->lua->tonumber(L, 3);
    int cols = api->lua->tonumber(L, 4);

    int len = api->lua->L_len(L, 2);
    if (len != (rows * cols))
    {
        return api->lua->L_error(L, "table size(%d) not match with rows*cols(%d)", len, rows * cols);
    }

    double* values = (double*)malloc(sizeof(double) * len);

    api->lua->pushnil(L);
    for (size_t i = 0; api->lua->next(L, 2) != 0; i++)
    {
        values[i] = api->lua->tonumber(L, -1);
        api->lua->pop(L, 1);
    }

    ImPlot::PlotHeatmap(label_id, values, rows, cols);
    free(values);

    return 0;
}

int imgui_luaopen_implot(lua_State *L)
{
    static const auto_luaL_Reg s_implot_method[] = {
        { "BeginPlot",      _implot_begin_plot },
        { "EndPlot",        _implot_end_plot },
        { "PlotBars",       _implot_plot_bars },
        { "PlotHeatmap",    _implot_plot_heatmap },
        { "PlotLine",       _implot_plot_line },
        { "PlotScatter",    _implot_plot_scatter },
        { "PlotShaded",     _implot_plot_shaded },
        { "PlotStairs",     _implot_plot_stairs },
        { "PlotStems",      _implot_plot_stems },
        { NULL,             NULL },
    };
    api->lua->L_newlib(L, s_implot_method);
    return 1;
}
