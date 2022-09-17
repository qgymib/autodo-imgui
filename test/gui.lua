local imgui = require("imgui")

local is_checked = false
local is_closed = false
local need_show = true
local show_demo = true

local function on_gui()
    if not need_show then
        return
    end

    is_open, need_show = imgui.Begin("Test window", need_show)

    if not is_open then
        imgui.End()
        return
    end

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

    if show_demo then
        show_demo = imgui.ShowDemoWindow()
    end

    imgui.ShowMetricsWindow()
    imgui.ShowStackToolWindow()

    imgui.End()
end

imgui.loop(on_gui, 2)
