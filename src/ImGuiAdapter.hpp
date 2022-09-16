#ifndef __IMGUI_ADAPTER_HPP__
#define __IMGUI_ADAPTER_HPP__

#include <autodo.h>

typedef struct imgui_ctx
{
    atd_coroutine_t*    co;

    atd_thread_t*       gui_thr;
    atd_sem_t*          sem;
    atd_sync_t*         nfy_gui_update;
    char*               window_title;

    uint64_t            now_time;
    uint64_t            last_frame;

    int                 looping;
    int                 fps;
    uint64_t            fps_delay;
} imgui_ctx_t;

void ImGuiAdapter(imgui_ctx_t* ctx, void(*callback)(imgui_ctx_t* ctx));

#endif
