#ifndef __IMGUI_ADAPTER_HPP__
#define __IMGUI_ADAPTER_HPP__

#include <autodo.h>

typedef struct imgui_ctx
{
    atd_coroutine_t*    co;

    auto_thread_t*      gui_thr;
    auto_sem_t*         sem;
    auto_notify_t*      nfy_gui_update;

    uint64_t            now_time;
    uint64_t            last_frame;

    int                 looping;
    int                 fps;
    uint64_t            fps_delay;

    struct
    {
        int             x;
        int             y;
        char*           title;
    } window;
} imgui_ctx_t;

AUTO_LOCAL void ImGuiAdapter(imgui_ctx_t* ctx, void(*callback)(imgui_ctx_t* ctx));

#endif
