# autodo-imgui
ImGui module for [autodo](https://github.com/qgymib/autodo).

## Usage

The module have only one public API:

```lua
require("imgui").loop(function(gui) end)
```

User pass a function to `loop()` which receive a control object for drawing widget.

```lua
local is_checked = false

require("imgui").loop(function(gui)
    gui:Begin("Test window")
    is_checked = gui:CheckBox("test box is_checked)
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
end)
```

## API

### AlignTextToFramePadding

```lua
gui.AlignTextToFramePadding()
```

Vertically align upcoming text baseline to FramePadding.y so that it will align properly to regularly framed items (call if you have text on a line before a framed item).

### Begin

```
bool gui.Begin(string name)
bool,bool gui.Begin(string name, bool show_close)
```

Push window to the stack and start appending to it.

The first parameter is the name of created window.

The optional second parameter is whether show a window-closing widget in the upper-right corner of the window, which clicking will set the second return value to false.

Begin() return false to indicate the window is collapsed or fully clipped, so you may early out and omit submitting anything to the window.

Always call a matching End() for each Begin() call, regardless of its return value.

### BeginChild

```lua
bool gui.BeginChild(string name)
```

Use child windows to begin into a self-contained independent scrolling/clipping regions within a host window. Child windows can embed their own child.

BeginChild() returns false to indicate the window is collapsed or fully clipped, so you may early out and omit submitting anything to the window.

Always call a matching EndChild() for each BeginChild() call, regardless of its return value.

### BeginGroup

```lua
gui.BeginGroup()
```

Lock horizontal starting position.

### BeginMenu

```lua
bool gui.BeginMenu(string label)
```

Create a sub-menu entry.

Only call EndMenu() if this returns true.

### BeginMenuBar

```lua
bool gui.BeginMenuBar()
```

Append to menu-bar of current window (requires ImGuiWindowFlags_MenuBar flag set on parent window).

### BulletText

```lua
gui.BulletText(string text)
```

Shortcut for Bullet()+Text().

### Button

```lua
bool gui.Button(string text)
```

Button.

### CheckBox

```lua
bool gui.CheckBox(string label, bool is_checked)
```

CheckBox.

### Dummy

```
gui.Dummy(float x, float y)
```

Add a dummy item of given size. unlike InvisibleButton(), Dummy() won't take the mouse click or be navigable into.

### End

```
gui.End()
```

Always call a matching End() for each Begin() call, regardless of its return value!

### EndChild

```lua
gui.EndChild()
```

Always call a matching EndChild() for each BeginChild() call, regardless of its return value.

### EndGroup

```
gui.EndGroup()
```

Unlock horizontal starting position + capture the whole group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)

### EndMenu

```lua
gui.EndMenu()
```

Only call EndMenu() if BeginMenu() returns true!

### EndMenuBar

```lua
gui.EndMenuBar()
```

Oonly call EndMenuBar() if BeginMenuBar() returns true!

### GetCursorPos

```lua
float,float gui.GetCursorPos()
```

Cursor position in window coordinates (relative to window position).

### GetCursorScreenPos

```lua
float,float gui.GetCursorScreenPos()
```

Cursor position in absolute coordinates (useful to work with ImDrawList API). generally top-left == GetMainViewport()->Pos == (0,0) in single viewport mode, and bottom-right == GetMainViewport()->Pos+Size == io.DisplaySize in single-viewport mode.

### GetFrameHeight

```lua
float gui.GetFrameHeight()
```

FontSize + style.FramePadding.y * 2.

### GetTextLineHeight

```
float gui.GetTextLineHeight()
```

FontSize.

### GetWindowPos

```lua
float,float gui.GetWindowPos()
```

Get current window position in screen space (useful if you want to do your own drawing via the DrawList API)

### GetWindowSize

```lua
float,float gui.GetWindowSize()
```

Get current window size

### Indent

```lua
gui.Indent(float indent_w)
```

Move content position toward the right, by indent_w, or style.IndentSpacing if indent_w <= 0.

### InputText

```
string gui.InputText(string label)
```

InputText.

### NewLine

```lua
gui.NewLine()
```

Undo a SameLine() or force a new line when in an horizontal-layout context.

### MenuItem

```lua
bool gui.MenuItem(string label, [string shortcut])
```

Return true when activated.

### PlotLines

```lua
gui.PlotLines(string label, float p1, [float p2, ...])
gui.PlotLines(string label, table)
```

Data Plotting.

### SameLine

```lua
gui.SameLine()
```

Call between widgets or groups to layout them horizontally. X position given in window coordinates.

### Separator

```lua
gui.Separator()
```

Separator, generally horizontal. inside a menu bar or in horizontal layout mode, this becomes a vertical separator.

### SetCursorPos

```lua
gui.SetCursorPos(float x, float y)
```

Set cursor position.

### SetNextWindowFocus

```lua
gui.SetNextWindowFocus()
```

Set next window to be focused / top-most. call before Begin().

### SetNextWindowPos

```lua
gui.SetNextWindowPos(float x, float y)
```

Set next window position. call before Begin(). use pivot=(0.5f,0.5f) to center on given point, etc.

### SetNextWindowSize

```lua
gui.SetNextWindowSize(float x, float y)
```

Set next window size. set axis to 0.0f to force an auto-fit on this axis. call before Begin(),

### ShowDemoWindow

```lua
bool gui.ShowDemoWindow()
```

Create Demo window. demonstrate most ImGui features. call this to learn about the library! try to make it always available in your application!

### ShowMetricsWindow

```lua
bool gui.ShowMetricsWindow()
```

Create Metrics/Debugger window. display Dear ImGui internals: windows, draw commands, various internal state, etc.

### ShowStackToolWindow

```lua
bool gui.ShowStackToolWindow()
```

Create Stack Tool window. hover items with mouse to query information about the source of their unique ID.

### SliderFloat

```lua
float gui.SliderFloat(string label, float value, float min, float max)
```

Adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.

### Spacing

```lua
gui.Spacing()
```

Add vertical spacing.

### Text

```lua
gui.Text(string text)
```

Text.

### TextColored

```lua
gui.TextColored(float x, float y, float z, float w, string text)
```

Shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();

### Unindent

```lua
gui.Unindent(float indent_w)
```

Move content position back to the left, by indent_w, or style.IndentSpacing if indent_w <= 0.
