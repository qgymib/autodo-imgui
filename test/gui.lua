local imgui = require("imgui")

local is_checked = false
local is_closed = false

local is_metrics_show = true
local function show_metrics_window()
    if is_metrics_show then
        is_metrics_show = imgui.ShowMetricsWindow()
    end
end

local is_demo_show = true
local function show_demo()
    if is_demo_show then
        is_demo_show = imgui.ShowDemoWindow()
    end
end

local is_test_show = true
local function show_test_window()
    if not is_test_show then
        return
    end

    _, is_test_show = imgui.Begin("Test window", true)

    is_checked = imgui.CheckBox("test box", is_checked)
    if imgui.Button("test button") then
        io.write("button clicked\n")
    end
    imgui.Text("text")
    imgui.TextColored(1, 1, 0, 1, "sf")
    imgui.BulletText("text2")
    local str = imgui.InputText("测试")
    if str ~= nil then
        io.write(str .. "\n")
    end

    imgui.PlotLines("fps", 1, 5, 12, 9, 2, 4, 0)

    imgui.End()
end

local function on_gui()
    show_demo()
    show_metrics_window()
    show_test_window()
end

local gui_token = imgui.loop({}, on_gui)
gui_token:await()
