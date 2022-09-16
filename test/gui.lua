local imgui = require("imgui")

local is_checked = false
local is_closed = false
local need_show = true
local show_demo = true

local function on_gui(gui)
    if not need_show then
        return
    end

    is_open, need_show = gui.Begin("Test window", need_show)

    if not is_open then
        gui.End()
        return
    end

    is_checked = gui.CheckBox("test box", is_checked)
    if gui.Button("test button") then
        io.write("button clicked\n")
    end
    gui.Text("text")
    gui.TextColored(1, 1, 0, 1, "sf")
    gui.BulletText("text2")
    local str = gui.InputText("测试")
    if str ~= nil then
        io.write(str .. "\n")
    end

    gui.PlotLines("fps", 1, 5, 12, 9, 2, 4, 0)

    if show_demo then
        show_demo = gui.ShowDemoWindow()
    end

    gui.ShowMetricsWindow()
    gui.ShowStackToolWindow()

    gui.End()
end

imgui.loop(on_gui)
