#include "ImGuiAdapter.hpp"
#include <implot.h>

#if defined(IMGUI_BACKEND_OPENGL3)
#   include <imgui_impl_opengl3.h>
#else
#   error no imgui backend
#endif

#if defined(IMGUI_BACKEND_GLFW)
#   include <GLFW/glfw3.h>
#   include <imgui_impl_glfw.h>
#elif defined(IMGUI_BACKEND_SDL)
#   include <SDL.h>
#   include <SDL_opengl.h>
#   include <imgui_impl_sdl.h>
#else
#   error no imgui backend
#endif

/* Font */
//extern "C" const unsigned int sarasa_compressed_data[];
//extern "C" const unsigned int sarasa_compressed_size;

void ImGuiAdapter(imgui_ctx_t* gui, void(*callback)(imgui_ctx_t* ctx))
{
#if defined(IMGUI_BACKEND_GLFW)
    if (!glfwInit())
#elif defined(IMGUI_BACKEND_SDL)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
#endif
    {
        assert(0);
    }

    const char* glsl_version = "#version 130";

#if defined(IMGUI_BACKEND_GLFW)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(IMGUI_BACKEND_SDL)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
#if defined(IMGUI_BACKEND_GLFW)
    GLFWwindow* window = glfwCreateWindow(gui->window.x, gui->window.y, gui->window.title, NULL, NULL);
    assert(window != NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
#elif defined(IMGUI_BACKEND_SDL)
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(gui->window.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        gui->window.x, gui->window.y, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
#if 0
    io.Fonts->AddFontFromMemoryCompressedTTF(sarasa_compressed_data,
        sarasa_compressed_size, 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
#endif
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
#if defined(IMGUI_BACKEND_GLFW)
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#elif defined(IMGUI_BACKEND_SDL)
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
#if defined(IMGUI_BACKEND_GLFW)
    while (!glfwWindowShouldClose(window))
#elif defined(IMGUI_BACKEND_SDL)
    bool done = false;
    while (!done)
#endif
    {
#if defined(IMGUI_BACKEND_GLFW)
        glfwPollEvents();
#elif defined(IMGUI_BACKEND_SDL)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
#endif

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
#if defined(IMGUI_BACKEND_GLFW)
        ImGui_ImplGlfw_NewFrame();
#elif defined(IMGUI_BACKEND_SDL)
        ImGui_ImplSDL2_NewFrame();
#endif
        ImGui::NewFrame();

        // GUI
        callback(gui);

        // Rendering
        ImGui::Render();
#if defined(IMGUI_BACKEND_GLFW)
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
#endif
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#if defined(IMGUI_BACKEND_GLFW)
        glfwSwapBuffers(window);
#elif defined(IMGUI_BACKEND_SDL)
        SDL_GL_SwapWindow(window);
#endif
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
#if defined(IMGUI_BACKEND_GLFW)
    ImGui_ImplGlfw_Shutdown();
#elif defined(IMGUI_BACKEND_SDL)
    ImGui_ImplSDL2_Shutdown();
#endif
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

#if defined(IMGUI_BACKEND_GLFW)
    glfwDestroyWindow(window);
    glfwTerminate();
#elif defined(IMGUI_BACKEND_SDL)
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}
