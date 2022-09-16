local imgui = require("imgui")

local is_checked = false

local function on_gui(gui)
    gui:Begin("Test window")
    is_checked = gui:CheckBox("test box", is_checked)
    if gui:Button("test button") then
        io.write("button clicked\n")
    end
    gui:Text("text")
    gui:BulletText("text2")
    local str = gui:InputText("测试")
    if str ~= nil then
        io.write(str .. "\n")
    end
    gui:End()
end

imgui.loop(on_gui)
