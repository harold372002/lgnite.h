// ============================================================================
// lgnite.h —— 纯 C++ / OpenGL 3.3 Core 单文件 2D 游戏开发库
// 版本: 1.0.0   License: MIT   标准: C++17
// ----------------------------------------------------------------------------
// 三步用法:
//   1) 在【某一个且仅一个】.cpp 文件中:
//        #define LGNITE_IMPLEMENTATION
//        #include "lgnite.h"
//   2) 其余所有文件直接 #include "lgnite.h"
//   3) 与本头文件同目录放置第三方单头文件:
//        stb_image.h  stb_truetype.h  miniaudio.h
//      并准备 GLFW(头文件 + 预编译库) 与系统 OpenGL 库
//      (Windows: glfw3.lib + glfw3.dll + opengl32.lib;
//       Linux:   -lglfw -lGL -lm -ldl -lpthread;
//       macOS:   -lglfw -framework OpenGL)
// ----------------------------------------------------------------------------
// 阶段能力一览(同一套 API, 无需换库/重构即可从 S0 走到 S4):
//   S0 入门图形 : Window / App / Renderer 画图形 / Color / Vec2
//   S1 基础游戏 : Texture / Sprite / Input / Camera2D / GameLoop / AudioClip
//   S2 小型游戏 : SpriteSheet / Animation / SoundPool / MusicPlayer / Font
//                 / Scene / SceneManager
//   S3 中型游戏 : Tilemap / ParticleSystem / ResourceManager / SaveSystem
//                 / InputMap / Camera2D 跟随
//   S4 大型游戏 : ECS(Entity/World/System) / ObjectPool / Config(Json)
//                 / Shader / EventBus / Profiler / 场景流式切换
// ----------------------------------------------------------------------------
// 设计约定:
//   - 坐标: 默认屏幕坐标, 原点左上角, x 向右, y 向下, 单位像素
//   - 角度: 一律为度(deg), 顺时针为正
//   - 失败返回 bool 并写日志(Log), 不抛异常
//   - 每个公共 API 上方的 [S?] 标签表示建议引入阶段
//   - 文件底部注释区附 S0~S4 五个可运行的最小示例
// ============================================================================
#ifndef LGNITE_H
#define LGNITE_H

#define LGNITE_VERSION "1.0.0"

// ---- 依赖: GLFW(窗口/输入) + 标准库 ----
#if defined(_WIN32)
#ifndef NOMINMAX
#define LGNITE_DEFINED_NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef LGNITE_DEFINED_NOMINMAX
#undef NOMINMAX
#undef LGNITE_DEFINED_NOMINMAX
#endif
#endif
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE   // 注意: 阻止 glfw 引入系统 GL 头, GL 由本库自加载
#endif
#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <typeindex>
#include <initializer_list>
#include <tuple>
#include <utility>
#include <chrono>
#include <algorithm>

namespace lgnite {

// ============================== 基础类型 [基础] ==============================

// [基础] 二维向量
struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float k) const { return {x * k, y * k}; }
    Vec2 operator/(float k) const { return {x / k, y / k}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    bool  operator==(Vec2 o) const { return x == o.x && y == o.y; }
    float length() const { return std::sqrt(x * x + y * y); }            // 模长
    Vec2  normalized() const { float l = length(); return l > 0 ? Vec2(x / l, y / l) : Vec2(0, 0); }
    float dot(Vec2 o) const { return x * o.x + y * o.y; }                // 点积
    float angle() const;                                                 // [基础] 方向角(度)
    Vec2 rotated(float deg) const;                                       // [基础] 旋转向量
    Vec2 perpendicular() const { return {-y, x}; }                       // [基础] 左法线
    static Vec2 lerp(Vec2 a, Vec2 b, float t);                            // 线性插值
    static float dist(Vec2 a, Vec2 b);                                   // [基础] 两点距离
};

// [扩展] 4x4 矩阵(列主序, 与 OpenGL 一致); 仅 2D 用途, 高级用户可用
struct Mat4 {
    float m[16];
    static Mat4 identity();
    static Mat4 ortho(float l, float r, float b, float t, float n = -1.f, float f = 1.f);
    static Mat4 translation(float x, float y);
    static Mat4 scaling(float x, float y);
    static Mat4 rotationZ(float deg);
    Mat4 operator*(const Mat4& o) const;   // 矩阵相乘: 结果 = this * o
};

// [基础] 轴对齐矩形, 同时承担碰撞盒职责; x,y 为左上角
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}
    bool contains(Vec2 p) const;              // 点是否在矩形内
    bool intersects(const Rect& o) const;     // [基础] AABB 碰撞检测
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    Rect inflated(float d) const;             // [基础] 四边扩展
    Rect offset(float dx, float dy) const;    // [基础] 平移
    Rect scaled(float k) const;               // [基础] 中心缩放
    Vec2 topLeft() const { return {x, y}; }
    Vec2 bottomRight() const { return {x + w, y + h}; }
    Vec2 size() const { return {w, h}; }
};

// [基础] RGBA 颜色, 分量 0~1
struct Color {
    float r = 1, g = 1, b = 1, a = 1;
    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.f) : r(r_), g(g_), b(b_), a(a_) {}
    static Color RGBA255(int r, int g, int b, int a = 255);  // 0~255 写法
    static Color Hex(uint32_t rgb, float a = 1.f);           // 0xRRGGBB 写法
    Color withAlpha(float na) const { return {r, g, b, na}; }
    static Color lerp(Color c0, Color c1, float t);          // 颜色插值(粒子常用)
    static Color HSL(float h, float s, float l, float a = 1); // [基础] HSL(色相为度)
    static Color HSV(float h, float s, float v, float a = 1); // [基础] HSV(色相为度)
    void toHSL(float& h, float& s, float& l) const;
    void toHSV(float& h, float& s, float& v) const;
    // 常用预设(静态常量, 可直接作默认参数, 无需括号; 定义为类外 inline 变量)
    static const Color White;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Orange;
    static const Color Gray;
    static const Color Transparent;
};
// C++17 inline 变量: 多翻译单元共享同一实例, 无重复定义问题
inline const Color Color::White       = {1, 1, 1, 1};
inline const Color Color::Black       = {0, 0, 0, 1};
inline const Color Color::Red         = {1, 0.2f, 0.2f, 1};
inline const Color Color::Green       = {0.2f, 0.9f, 0.3f, 1};
inline const Color Color::Blue        = {0.25f, 0.5f, 1, 1};
inline const Color Color::Yellow      = {1, 0.9f, 0.2f, 1};
inline const Color Color::Cyan        = {0.2f, 0.9f, 1, 1};
inline const Color Color::Magenta     = {1, 0.3f, 1, 1};
inline const Color Color::Orange      = {1, 0.6f, 0.15f, 1};
inline const Color Color::Gray        = {0.5f, 0.5f, 0.5f, 1};
inline const Color Color::Transparent = {0, 0, 0, 0};

// [进阶] 组合变换(位置/旋转/缩放), ECS 中常作 Component 使用
struct Transform {
    Vec2  pos{0, 0};
    float rotation = 0;       // 度
    Vec2  scale{1, 1};
    Mat4 matrix() const;      // T * R * S
};

// ============================== 窗口 [基础] ==============================

// [基础] 窗口与 GL 上下文封装; 一般整个程序只创建一个
class Window {
    GLFWwindow* handle_ = nullptr;
    int w_ = 0, h_ = 0;
    int savedX_ = 100, savedY_ = 100, savedW_ = 800, savedH_ = 600; // 全屏前的窗口位置/尺寸
    bool fullscreen_ = false;
public:
    Window() = default;
    ~Window();
    // 创建窗口(内部自动初始化 GLFW 并请求 OpenGL 3.3 Core); 失败返回 false
    bool create(int w, int h, const char* title, bool vsync = true);
    void destroy();
    bool shouldClose() const;            // 用户点关闭按钮后为 true
    void close();                        // 主动请求关闭
    void pollEvents();                   // 处理系统事件(每帧调用)
    void swapBuffers();                  // 交换前后缓冲(每帧末尾调用)
    void getSize(int& w, int& h) const { w = w_; h = h_; }
    float aspect() const { return h_ ? (float)w_ / (float)h_ : 1.f; }
    void setSize(int w, int h);
    void setTitle(const char* title);
    bool isFocused() const;
    void setFullscreen(bool fullscreen);        // [进阶] 切换全屏/窗口化(自动记忆窗口位置)
    bool isFullscreen() const { return fullscreen_; }
    bool setIcon(const char* path);             // [进阶] 设置窗口图标(png 等, 需 stb_image)
    float getDpiScale() const;                  // [进阶] DPI 缩放系数(高分辨率屏 > 1)
    GLFWwindow* handle() const { return handle_; }  // 原生句柄(高级用法)
};

// ============================== OpenGL 环境 [基础] ==============================

// [基础] GL 函数动态加载与诊断; 调用顺序: Window::create() -> gl::init() -> 渲染
namespace gl {
    bool init();                              // 加载全部 GL 函数并设置默认状态
    bool ready();                             // GL 函数是否已就绪( init 成功后为 true )
    void checkError(const char* tag);         // 打印自上次以来的 GL 错误(调试用)
    const char* version();                    // GL 版本字符串
}

// ============================== 渲染 [基础] ==============================

class Texture;
struct Sprite;
class Camera2D;
class Font;

// [基础] 批渲染器(全局静态): 所有图形/精灵/文字最终都走这里
// 注意: 坐标原点为左上角; 一帧内任意顺序调用, 末尾 flush() 统一上屏
class Renderer {
public:
    enum class LineCap { Butt, Round, Square };
    static bool init();                 // gl::init() 之后调用
    static void shutdown();
    static void clear(const Color& c);  // 清屏
    // 设置视口(像素)与逻辑分辨率; App/GameLoop 每帧自动调用
    static void setViewport(int w, int h);
    // 设置摄像机; 传 nullptr 回到屏幕坐标(左上角原点)
    static void setCamera(const Camera2D* cam);
    static void drawRect(float x, float y, float w, float h, const Color& c, float rotDeg = 0);
    static void drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& c, LineCap cap = LineCap::Round);
    static void drawCircle(float cx, float cy, float radius, const Color& c, int segments = 32);
    static void drawCircleAA(float cx, float cy, float radius, const Color& c, int segments = 64);
    static void drawRectOutline(float x, float y, float w, float h, float thickness, const Color& c);
    static void drawCircleOutline(float cx, float cy, float radius, float thickness, const Color& c, int segments = 64);
    static void drawRectGradient(float x, float y, float w, float h, Color topLeft, Color topRight,
                                 Color bottomLeft, Color bottomRight, float rotDeg = 0);
    static void drawRoundRect(float x, float y, float w, float h, float radius, const Color& c, int cornerSegments = 8);
    static void drawNineSlice(const Texture& tex, float x, float y, float w, float h,
                              float left, float right, float top, float bottom);
    // 绘制纹理区域; (x,y) 为左上角, rotDeg 绕中心旋转, u0/v0/u1/v1 为归一化 UV
    static void drawTexture(const Texture& tex, float x, float y, float w, float h,
                            const Color& tint = Color::White, float rotDeg = 0,
                            float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1);
    static void drawTextureFlipped(const Texture& tex, float x, float y, float w, float h,
                                   bool flipX, bool flipY, const Color& tint = Color::White, float rotDeg = 0);
    static void setBlendAdditive(bool additive);
    static int getDrawCallCount();
    static void resetStats();
    static void debugDrawRect(const Rect& r, const Color& c = Color::Green);
    static void debugDrawCircle(Vec2 center, float radius, const Color& c = Color::Green);
    static void debugDrawLine(Vec2 a, Vec2 b, const Color& c = Color::Green);
    static void debugDrawGrid(float spacing, int cols, int rows, const Color& c = Color::Gray);
    static void drawSprite(const Sprite& s);   // [基础] 画一个精灵
    // [进阶] 画文字(内部转调 Font::drawText, 省得以渲染器为中心时还要绕道)
    static void drawText(const Font& font, float x, float y, const char* text,
                         const Color& c, float scale = 1);
    static void drawTextf(const Font& font, float x, float y, const Color& c, float scale,
                          const char* fmt, ...);
    static void flush();                        // 帧尾统一提交(App/GameLoop 自动调用)
};

// [基础] 纹理(图片); 内部为 GL 纹理对象, 移动语义, 不可拷贝
class Texture {
    unsigned int id_ = 0;
    int w_ = 0, h_ = 0;
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;
    // 从文件加载(png/jpg...)
    // 注意: 方向正确性依赖以下约定, 改动任一环节需重新验证方向:
    //       1) stb_image 解码数据首行为图像顶部(屏幕坐标 y 向下);
    //       2) glTexImage2D 把首行放在 GL 纹理 v=0(GL 纹理 v=0 在底部);
    //       3) pushRectVerts 用 (u0,v0) 作为四边形左上角 UV。
    //       三者叠加使图片在 2D 屏幕坐标下方向正确, 故默认 flipY=false;
    //       仅当图片数据本身是 OpenGL 顺序(首行为底部)时才需 flipY=true。
    bool load(const char* path, bool flipY = false);
    bool loadFromMemory(const unsigned char* data, int len);// 从内存缓冲解码
    bool create(int w, int h, const unsigned char* rgba, bool repeat = false); // [基础] RGBA 像素创建/可平铺
    // [进阶] 创建空纹理(无初始像素), 用于离屏渲染颜色附件(RenderTarget)
    bool createEmpty(int w, int h, bool repeat = false);
    void destroy();
    void setFilter(bool smooth);    // true=线性(缩放平滑) false=最近邻(像素风)
    unsigned int id() const { return id_; }
    int width()  const { return w_; }
    int height() const { return h_; }
    bool valid() const { return id_ != 0; }
};

// [基础] 精灵: 一块可移动/旋转/染色的图片; pos 为左上角, 旋转绕中心
struct Sprite {
    const Texture* tex = nullptr;
    Vec2  pos{0, 0};
    Vec2  size{1, 1};            // autoSize=false 时使用此尺寸(默认 1x1, 通常先设 size 再关 autoSize)
    bool  autoSize = true;       // true = 尺寸跟随纹理(此时忽略 size)
    Vec2  scale{1, 1};
    float rotation = 0;          // 度, 绕中心
    Vec2 anchor{0.5f, 0.5f};    // [基础] 旋转锚点(相对尺寸)
    Color color = Color::White;  // 染色(White = 原图)
    Rect  uv{0, 0, 1, 1};        // 归一化 UV 区域
    bool  flipX = false, flipY = false;
    void draw() const;                       // 等价 Renderer::drawSprite(*this)
    Rect bounds() const;                     // 碰撞用 AABB(不含旋转)
    Rect boundsRotated() const;               // [基础] 旋转后的包围盒
};
bool circleCollide(Vec2 a, float ra, Vec2 b, float rb); // [基础] 圆形碰撞

// [进阶] 精灵表(均匀网格切图); frame 编号按行优先
struct FrameUV { float u0, v0, u1, v1; };
class SpriteSheet {
public:
    const Texture* tex = nullptr;
    int frameW = 0, frameH = 0, cols = 0, rows = 0;
    SpriteSheet() = default;
    SpriteSheet(const Texture& t, int frameW_, int frameH_) { set(t, frameW_, frameH_); }
    void set(const Texture& t, int frameW_, int frameH_);
    int count() const { return cols * rows; }      // 总帧数
    FrameUV frame(int index) const;                // 取某帧的 UV
};

// [进阶] 帧动画播放器(配合 SpriteSheet 使用)
class Animation {
public:
    std::vector<int> frames;   // 帧号序列(对应 SpriteSheet.frame 的 index)
    float fps = 8;
    bool  loop = true;
    bool  playing = true;
    float time = 0;
    int   cursor = 0;
    Animation() = default;
    Animation(std::initializer_list<int> f, float fps_ = 8, bool loop_ = true)
        : frames(f), fps(fps_), loop(loop_) {}
    void setRange(int first, int last);    // 快捷设置连续帧 [first,last]
    void play()  { playing = true; }
    void pause() { playing = false; }
    void reset() { time = 0; cursor = 0; playing = true; }
    void update(float dt);                 // 每帧推进
    int  current() const;                  // 当前帧号
    bool finished() const;                 // 非循环动画是否已播完
};

// [基础] 2D 摄像机: 位置(视野中心)/缩放/旋转, 支持平滑跟随与边界钳制
class Camera2D {
public:
    Vec2  pos{0, 0};              // 视野中心的世界坐标
    float zoom = 1;
    float zoomMin = 0.1f, zoomMax = 10.f;
    float rotation = 0;           // 度
    float followSmooth = 6;       // 跟随平滑系数(越大越紧)
    bool  clampEnabled = false;   // 是否限制视野不超出 clampRect
    Rect  clampRect{0, 0, 0, 0};  // 世界边界
    void setZoom(float z);
    void shake(float intensity, float duration); // [进阶] 摄像机震动
    void follow(Vec2 target, float dt, float screenW = 0, float screenH = 0);
    void snapTo(Vec2 target) {
        pos = target;
        shakeTime_ = shakeDuration_ = shakeIntensity_ = 0;
        shakeOffset_ = {0, 0};
    }
    Mat4 viewProj(float screenW, float screenH) const;          // 视图投影矩阵
    Vec2 screenToWorld(Vec2 s, float screenW, float screenH) const;
    Vec2 worldToScreen(Vec2 p, float screenW, float screenH) const;
private:
    float shakeTime_ = 0, shakeDuration_ = 0, shakeIntensity_ = 0;
    Vec2 shakeOffset_{0, 0};
};

// [进阶] 位图字体(stb_truetype 离线烘焙); 移动语义, 不可拷贝
// 默认烘焙 ASCII 32~126; 需要中文等字符时用带 codepoints 的重载指定字符集
//   例: const uint32_t cps[] = {0x4E2D, 0x6587, ...}; font.load("f.ttf", 24, cps, 2);
//   常用做法: 收集游戏中实际用到的汉字码点(几千个以内), 一次烘焙。
class Font {
    Texture atlas_;
    void* pdata_ = nullptr;      // 内部: stbtt_packedchar[count]
    std::vector<uint32_t> cps_;  // 码点表(与 pdata_ 一一对应)
    std::unordered_map<uint32_t, int> idx_;  // 码点 -> 数组下标
    int   atlasW_ = 0, atlasH_ = 0;
    float size_ = 16;
    bool  ok_ = false;
    int oversampleX_ = 2, oversampleY_ = 2;
    void drawTextV(float x, float y, const Color& c, float scale, const char* fmt, va_list args) const;
    friend class Renderer;
public:
    enum class Align { Left, Center, Right };
    float lineSpacing = 1.f, letterSpacing = 0.f;
    Font() = default;
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&& o) noexcept;
    Font& operator=(Font&& o) noexcept;
    bool load(const char* path, float pixelSize = 16);       // 加载 ttf/otf(默认 ASCII)
    // [进阶] 指定字符集加载(中文/自定义); codepoints 为 Unicode 码点数组
    bool load(const char* path, float pixelSize, const uint32_t* codepoints, int count);
    bool loadFromMemory(const unsigned char* data, int len, float pixelSize,
                        const uint32_t* codepoints = nullptr, int count = 0);
    bool loadFromString(const char* path, float pixelSize, const char* sampleText); // [进阶] 按样本文字烘焙
    bool loadCJK(const char* path, float pixelSize); // [进阶] 烘焙中日韩统一汉字区
    void setOversample(int x, int y); // [进阶] 设置字体超采样
    // 绘制 UTF-8 文字(支持 \n 换行); 未烘焙的字符自动跳过
    // 注意: 文字 upright 依赖与 Texture::load 相同的约定(stb 位图首行->GL v=0->四边形顶部 UV),
    //       改动 UV 顺序或位图上传方式需重新验证方向
    void drawText(float x, float y, const char* utf8, const Color& c, float scale = 1) const;
    void drawTextOutline(float x, float y, const char* utf8, const Color& fillColor,
                         const Color& outlineColor, float outlineThickness = 1, float scale = 1) const; // 多次绘制描边，大量文字可考虑 SDF 字体
    void drawTextAligned(float x, float y, const char* utf8, const Color& c,
                         Align align = Align::Left, float scale = 1) const;
    void drawTextShadow(float x, float y, const char* utf8, const Color& fillColor,
                        Color shadowColor = { 0, 0, 0, 0.5f }, float offsetX = 2, float offsetY = 2,
                        float scale = 1) const;
    void drawTextf(float x, float y, const Color& c, float scale, const char* fmt, ...) const;
    float textWidth(const char* utf8, float scale = 1) const;
    // 计算文本高度(像素): 单行返回 size*scale, 含 \n 的多行返回 行数*size*scale
    float textHeight(const char* utf8, float scale = 1) const;
    float size() const { return size_; }
    bool  valid() const { return ok_; }
};

// [进阶] 粒子系统: 配置好参数后 update/draw 即可; emit() 手动爆发
struct Particle {
    Vec2  pos, vel;
    float life = 0, maxLife = 1;
    float size0 = 4, size1 = 0;    // 尺寸随寿命插值
    Color c0 = Color::White, c1 = Color::Transparent;
    float rotation = 0, rotationVel = 0;
};
class ParticleSystem {
public:
    enum class Blend { Alpha, Additive };
    enum class EmitShape { Point, Circle, Rect, Line };
    enum class Ease { Linear, EaseIn, EaseOut, EaseInOut };
    Blend blend = Blend::Alpha;
    EmitShape shape = EmitShape::Point;
    Ease sizeEase = Ease::Linear, colorEase = Ease::Linear;
    float shapeRadius = 0;
    Vec2 shapeSize{0, 0}, shapeLineEnd{0, 0};
    float rotationMin = 0, rotationMax = 0, rotationVelMin = 0, rotationVelMax = 0;
    Vec2  pos{0, 0};               // 发射点
    bool  emitting = true;         // 是否持续发射
    float rate = 20;               // 每秒发射数
    int   maxParticles = 512;
    Vec2  velMin{-50, -50}, velMax{50, 50};  // 初速度随机范围
    float lifeMin = 0.4f, lifeMax = 1.0f;
    float size0 = 6, size1 = 0;
    Color color0 = Color::White, color1 = Color::Transparent;
    Vec2  gravity{0, 0};           // 粒子重力
    const Texture* tex = nullptr;  // 非空时绘制纹理粒子(为空则画方块)
    void emit(int n = 1);          // 立即发射 n 个(爆炸等一次性效果)
    void update(float dt);
    void draw() const;
    int aliveCount() const { return (int)parts_.size(); }
private:
    std::vector<Particle> parts_;
    float acc_ = 0;
};

// [扩展] 骨骼动画: 仅提供接口与简化实现, 供后续接入完整 2D 骨骼系统 [简化]
// 说明: 完整骨骼(骨层级蒙皮/动画混合/IK)超出本库范围; 此处可记录骨骼树并查询世界坐标,
//       但不做蒙皮渲染。生产项目建议接入 Spine/DragonBones 等第三方运行时。
struct Bone {
    std::string name;
    int parent = -1;          // 父骨骼索引(-1 表示根)
    Vec2 pos{0, 0};           // 相对父骨骼的本地位置
    float rotation = 0;       // 本地旋转(度)
    float scale = 1;
};
class Skeleton {
public:
    std::vector<Bone> bones;
    int  addBone(const char* name, int parent);      // 添加骨骼, 返回索引
    void setPose(int index, Vec2 pos, float rotDeg); // 设置某骨骼姿态
    void update(float dt);                           // [简化] 无动画曲线/骨骼混合
    Vec2 boneWorldPos(int index) const;              // 计算骨骼世界坐标(仅平移累加)
    void draw() const;                               // [简化] 无蒙皮网格, 不绘制
};

// [扩展] GPU 粒子: 仅接口与空实现; 常规粒子请用上方的 ParticleSystem(CPU) [简化]
// 说明: 本库粒子默认走 CPU 批渲染, 计算着色器(GPU)版本留作扩展点, 接口恒定返回 false。
class GpuParticleSystem {
public:
    bool init(int maxParticles);     // [简化] 恒返回 false(未启用 GPU 后端)
    void emit(Vec2 pos, int count);  // [简化] 空实现
    void update(float dt);           // [简化] 空实现
    void draw() const;               // [简化] 空实现
    bool enabled() const { return false; }
};

// [扩展] 自定义着色器(高级); 注意: Renderer 内部使用自带着色器, 此类供自定义管线
class Shader {
    unsigned int prog_ = 0;
public:
    Shader() = default;
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;
    bool compile(const char* vsSrc, const char* fsSrc);  // 编译并链接
    void destroy();
    void use() const;
    void setInt(const char* name, int v) const;
    void setFloat(const char* name, float v) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setMat4(const char* name, const Mat4& m) const;
    unsigned int program() const { return prog_; }
    bool valid() const { return prog_ != 0; }
};

// [进阶] 瓦片地图: 整数网格 + 图集; -1 表示空块
class Tilemap {
public:
    enum class AutotileMode { Bit4, Bit16 };
    struct Layer { std::vector<int> data; float parallax = 1.f; bool visible = true; std::string name; };
    void create(int w, int h, int tileSize);            // 创建空地图
    void setTileset(const Texture& tex, int cols);      // 正方形网格图集(每行 cols 块)
    void setTileset(const Texture& tex, int cols, int rows); // 矩形网格图集(cols x rows)
    void set(int x, int y, int id);                     // 设置某格瓦片 id(-1 清空)
    int  get(int x, int y) const;
    void addLayer(const char* name); // [进阶] 添加图层
    void set(int layer, int x, int y, int id);
    int get(int layer, int x, int y) const;
    void setLayerParallax(const char* name, float p); // 视差按摄像机位置计算；旋转摄像机时建议 parallax=1
    void setLayerVisible(const char* name, bool visible);
    bool hasLayer(const char* name) const; // [进阶] 查询图层是否存在
    void setTileSolid(int id, bool solid);
    bool isTileSolid(int x, int y) const;
    bool isSolidAt(Vec2 worldPos) const;
    void setAutotile(bool enable, AutotileMode mode = AutotileMode::Bit4) { autotile_ = enable; autotileMode_ = mode; }
    int autotileId(int x, int y) const;
    bool loadCSV(const char* path);                     // 从逗号分隔文本载入
    bool loadTiledJson(const char* path);               // [进阶] 从 Tiled 编辑器导出的 JSON 载入
    void draw() const;                                  // 绘制全部非空块
    int width()  const { return w_; }
    int height() const { return h_; }
    int tileSize() const { return tile_; }
    Rect tileBounds(int x, int y) const;                // 某格的世界 AABB(碰撞用)
private:
    int w_ = 0, h_ = 0, tile_ = 32, cols_ = 1, rows_ = 1;
    float cellW_ = 0, cellH_ = 0;       // 图集中单块像素尺寸
    const Texture* tileset_ = nullptr;
    std::vector<int> data_;
    std::vector<Layer> layers_;
    std::unordered_set<int> solidIds_;
    bool autotile_ = false;
    AutotileMode autotileMode_ = AutotileMode::Bit4;
    std::vector<FrameUV> uv_;           // 预计算的每块 UV(setTileset 时生成)
    mutable bool warned_ = false;       // 本实例 id 越界是否已警告(不跨实例共享)
};

// ============================== 碰撞与物理 [进阶] ==============================

// [基础] AABB(轴对齐矩形) 碰撞检测
bool aabbOverlap(const Rect& a, const Rect& b);
// [基础] 点是否在矩形内
bool pointInRect(Vec2 p, const Rect& r);
// [基础] 圆与圆碰撞(circleCollide 的语义化别名)
bool circleCircle(Vec2 a, float ra, Vec2 b, float rb);
// [基础] 圆与 AABB 矩形碰撞
bool circleRect(Vec2 center, float radius, const Rect& r);

// [进阶] 碰撞结果: moveAndCollide 修正后的状态
struct CollisionInfo {
    bool hit = false;      // 本帧是否发生碰撞
    Vec2 normal{0, 0};     // 碰撞法线(指向被阻挡的反方向)
    Vec2 position{0, 0};   // 修正后的位置
};

// [进阶] 2D 刚体(平台跳跃/重力用): 位置/尺寸/速度/重力
struct RigidBody2D {
    Vec2 position{0, 0};      // 左上角
    Vec2 size{16, 16};
    Vec2 velocity{0, 0};
    float gravity = 900.f;    // 重力加速度(像素/秒^2), 0 表示无重力
    bool onGround = false;    // 是否站在地面(每次 moveAndCollide 后更新)
    bool solid = true;        // 是否参与瓦片碰撞
};

// [进阶] 应用重力(通常在 moveAndCollide 前调用)
void applyGravity(RigidBody2D& body, float dt);

// [进阶] 移动并碰撞: 一行解决角色移动 + 瓦片碰撞 + 平台落地
// 按 velocity*dt 移动, 与 map 中标记为 solid 的瓦片做 AABB 分离(X 轴->Y 轴),
// 返回碰撞信息, 并把修正后的位置写回 body.position、更新 body.onGround
CollisionInfo moveAndCollide(RigidBody2D& body, const Tilemap& map, float dt);

// ============================== 角色 [进阶] ==============================

// [进阶] 角色基类: 传统 OOP 写法(Sprite 体系), 与 ECS 可共存可迁移
class Actor {
public:
    Vec2   pos{0, 0};
    Sprite sprite;
    bool   alive = true;
    virtual ~Actor() = default;
    virtual void update(float dt) { (void)dt; }
    virtual void draw() { sprite.pos = pos; sprite.draw(); }
};

// [进阶] 角色容器: 统一管理一批 Actor 的更新/绘制/回收
class ActorGroup {
    std::vector<std::unique_ptr<Actor>> items_;
public:
    template<typename T, typename... Args>
    T& emplace(Args&&... args) {                        // 创建并纳入管理
        items_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return *static_cast<T*>(items_.back().get());
    }
    void add(std::unique_ptr<Actor> a) { items_.push_back(std::move(a)); }
    void update(float dt);       // 逐个 update
    void draw();                 // 逐个 draw
    void removeDead();           // 回收 alive==false 的
    void clear() { items_.clear(); }
    size_t size() const { return items_.size(); }
    Actor* operator[](size_t i) const { return items_[i].get(); }
};

// ============================== 音频 [基础] ==============================

// [基础] 音频引擎(全局单例, 基于 miniaudio); App 会自动初始化
class AudioEngine {
    void* engine_ = nullptr;     // 内部: ma_engine
    bool ok_ = false;
public:
    static AudioEngine& instance();
    bool init();                 // 初始化音频设备
    void shutdown();
    void setMasterVolume(float v);               // 0~1 主音量
    void updateFades(float dt);                  // [进阶] 推进淡入淡出(App 自动调用)
    bool play(const char* path);                 // 一次性播放(不缓存, 适合零散音效)
    bool ok() const { return ok_; }
    void* handle() const { return engine_; }     // 原生句柄(高级用法)
};

// [扩展] 音频总线: 一组声音统一调音量(如"音效"/"音乐"分组)
class AudioBus {
    void* group_ = nullptr;      // 内部: ma_sound_group
public:
    AudioBus() = default;
    ~AudioBus();
    AudioBus(const AudioBus&) = delete;
    AudioBus& operator=(const AudioBus&) = delete;
    bool create();               // AudioEngine 初始化后调用
    void setVolume(float v);
    void* handle() const { return group_; }
};

// [基础] 音频剪辑: 加载一个声音文件; 单实例播放(重叠播放请用 SoundPool)
class AudioClip {
    void* sound_ = nullptr;      // 内部: ma_sound
    bool ok_ = false;
    float fadeStart_ = 0, fadeDuration_ = 0, fadeFrom_ = 0, fadeTo_ = 0, volume_ = 1;
    void updateFade(float dt);
    friend class AudioEngine;
public:
    AudioClip() = default;
    ~AudioClip();
    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
    AudioClip(AudioClip&& o) noexcept;
    AudioClip& operator=(AudioClip&& o) noexcept;
    bool load(const char* path, AudioBus* bus = nullptr);  // wav/mp3/ogg/flac
    // 播放; restart=true 从头播(默认), false 不重置位置(配合 pause 继续播)
    void play(float volume = 1, bool restart = true);
    void stop();
    void pause();   // 仅暂停保持位置; 继续播请用 play(1.0f, false)
    bool isPlaying() const;
    void setVolume(float v);
    void setLoop(bool loop);
    void fadeIn(float duration, float targetVolume = 1);
    void fadeOut(float duration);
    float getPosition() const;
    float getDuration() const;
    void seek(float seconds);
    void setPosition(float x, float y);
    void setListenerPosition(float x, float y);
    void setDistanceAttenuation(bool enable, float refDist = 100, float maxDist = 1000);
    bool valid() const { return ok_; }
};

// [进阶] 音效池: 同一音效多路并发(枪声/拾取等高频短音效)
// 注意: 非线程安全(内部 startTimes_ 等状态无锁); 请在游戏主线程调用 play/load/stopAll
class SoundPool {
    std::vector<void*> sounds_;  // 内部: ma_sound[voices]
    std::vector<double> startTimes_;   // 各声道开始时间(抢占最旧的)
    int cur_ = 0;
public:
    SoundPool() = default;
    ~SoundPool();
    SoundPool(const SoundPool&) = delete;
    SoundPool& operator=(const SoundPool&) = delete;
    bool load(const char* path, int voices = 4, AudioBus* bus = nullptr);
    void play(float volume = 1);   // 轮询找一个空闲声道播放
    void stopAll();
    bool valid() const { return !sounds_.empty(); }
};

// [进阶] 音乐播放器: 流式播放长音频(BGM), 支持循环
class MusicPlayer {
    void* sound_ = nullptr;      // 内部: ma_sound (stream)
    bool ok_ = false;
public:
    MusicPlayer() = default;
    ~MusicPlayer();
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;
    bool open(const char* path, AudioBus* bus = nullptr);
    void play();
    void pause();
    void stop();
    void setLoop(bool loop);     // BGM 一般 true
    void setVolume(float v);
    bool isPlaying() const;
    float getPosition() const;
    float getDuration() const;
    void seek(float seconds);
    const void* rawHandle() const { return sound_; } // [进阶] 高级 miniaudio 访问; 频谱需自行接 miniaudio 解码回调
    bool valid() const { return ok_; }
};

// ============================== 输入 [基础] ==============================

// [基础] 轮询式输入(全局静态); 键码直接用 GLFW_KEY_* / GLFW_MOUSE_BUTTON_*
// 注意: Input::update 由 App/GameLoop 每帧自动调用; 手动循环时需自行调用
class Input {
public:
    static void update(Window& win);        // 每帧开头采样
    static bool isKeyDown(int key);         // 按住
    static bool isKeyPressed(int key);      // 本帧刚按下
    static bool isKeyReleased(int key);     // 本帧刚松开
    static bool isMouseDown(int btn);
    static bool isMousePressed(int btn);
    static bool isMouseReleased(int btn);
    static Vec2 mousePos();                 // 窗口内像素坐标
    static float scrollDelta();             // 本帧滚轮增量
    // ---- 手柄 [进阶](可选, 基于 GLFW 手柄映射) ----
    // 注意: pad 为 GLFW joystick id(非固定 0~15, 取决于系统枚举);
    //       用 for (int i = 0; i < 16; ++i) if (isGamepadConnected(i)) 枚举已连接手柄
    static bool isGamepadConnected(int pad);
    static bool isGamepadButtonDown(int pad, int btn);  // btn 用 GLFW_GAMEPAD_BUTTON_*
    static float gamepadAxis(int pad, int axis);        // axis 用 GLFW_GAMEPAD_AXIS_*, 返回 -1~1
};

// [进阶] 动作映射: 把"跳跃/开火"等动作绑定到若干按键, 方便改键
class InputMap {
    std::unordered_map<std::string, std::vector<int>> binds_;
    std::unordered_map<std::string, std::vector<int>> gamepadBinds_, axisBinds_;
    std::unordered_map<std::string, std::vector<float>> axisThresholds_;
public:
    void bind(const char* action, int key);      // 同一动作可绑多键
    void unbind(const char* action, int key);
    void clear(const char* action);
    void bindGamepad(const char* action, int button);
    void bindGamepadAxis(const char* action, int axis, float threshold = 0.5f);
    bool down(const char* action) const;         // 任一键按住
    bool pressed(const char* action) const;      // 任一键刚按下
    std::string serialize() const;               // [扩展] 序列化为 JSON(键鼠动作映射, 用于改键存档)
    bool deserialize(const std::string& data);   // [扩展] 从 JSON 恢复动作映射
};

// ============================== 场景 [进阶] ==============================

// [进阶] 场景基类: 一个关卡/界面一个子类
class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() {}          // 切入时
    virtual void onExit() {}           // 切出时
    virtual void onUpdate(float dt) { (void)dt; }
    virtual void onDraw() {}
};

// [进阶] 场景管理器: 按名字注册/切换; switchTo 延迟到下一帧生效(避免回调中自毁)
class SceneManager {
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    std::string pending_;
    Scene* cur_ = nullptr;
    Scene* transPrev_ = nullptr;
public:
    enum class Transition { None, Fade, Slide };
private:
    Transition trans_ = Transition::None;
    float transTime_ = 0, transDuration_ = 0;
    std::string transTarget_;
    bool transSwitched_ = false;
public:
    void add(const char* name, std::unique_ptr<Scene> s);
    void remove(const char* name);
    void switchTo(const char* name);   // 请求切换(下一帧 update 时生效)
    void setTransition(Transition t, float duration = 0.3f);
    void update(float dt);             // 处理挂起切换 + 当前场景 onUpdate
    void draw();                       // 当前场景 onDraw
    Scene* current() const { return cur_; }
    Scene* get(const char* name) const;
};

// ============================== ECS [扩展] ==============================

// [扩展] 实体 id(纯整数, 组件数据存于 World)
using Entity = uint32_t;
constexpr Entity NullEntity = 0;
struct Json;

// [扩展] 系统基类: 每帧对 World 中感兴趣的组件做批量处理
struct System {
    virtual ~System() = default;
    virtual void update(class World& world, float dt) = 0;
};
// [基础] 日志(printf 风格); 可用 setSink 接管输出(写文件/UI 控制台等)
class Log {
public:
    enum Level { Info, Warn, Error };
    using Sink = std::function<void(Level, const char*)>;
    static void setSink(Sink s);
    static void info (const char* fmt, ...);
    static void warn (const char* fmt, ...);
    static void error(const char* fmt, ...);
};
// [扩展] ECS 世界: 实体管理 + 组件存储 + 系统调度
// 用法: world.create(); world.add<Transform>(e); world.each<Transform,Sprite>(...);
class World {
    struct PoolBase {
        virtual ~PoolBase() = default;
        virtual void erase(Entity e) = 0;
        virtual bool contains(Entity e) const = 0;
    };
    template<typename T>
    struct Pool : PoolBase {
        std::unordered_map<Entity, T> data;
        void erase(Entity e) override { data.erase(e); }
        bool contains(Entity e) const override { return data.count(e) != 0; }
    };
    Entity next_ = 1;
    std::unordered_set<Entity> alive_;   // 存活的实体(只存键不存值, 比 map<Entity,bool> 省内存)
    std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> pools_;
    std::vector<std::unique_ptr<System>> systems_;
    std::unordered_map<std::type_index, std::string> componentNames_;
    std::unordered_map<std::type_index, std::function<std::string(const World&, Entity)>> serializers_;
    std::unordered_map<std::string, std::function<void(World&, Entity, const Json&)>> deserializers_;

    template<typename T>
    Pool<T>& poolFor() {   // 取(或创建)某组件池
        auto ti = std::type_index(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end())
            it = pools_.emplace(ti, std::make_unique<Pool<T>>()).first;
        return *static_cast<Pool<T>*>(it->second.get());
    }
public:
    template<typename T>
    void registerComponent(const char* name, std::function<std::string(const T&)> toJson,
                           std::function<T(const Json&)> fromJson) {
        // toJson 必须返回合法 JSON 字符串(例如使用 Json::dump() 生成)。
        const std::string key = name ? name : "";
        const std::type_index ti(typeid(T));
        if (key.empty() || !toJson || !fromJson) { Log::error("[World] 组件序列化注册无效"); return; }
        for (const auto& registered : componentNames_)
            if (registered.first != ti && registered.second == key) {
                Log::error("[World] 组件序列化名称重复: %s", key.c_str()); return;
            }
        componentNames_[ti] = key;
        serializers_[ti] = [toJson](const World& world, Entity e) {
            const T* value = world.get<T>(e);
            return value ? toJson(*value) : std::string();
        };
        deserializers_[key] = [fromJson](World& world, Entity e, const Json& value) {
            world.add<T>(e, fromJson(value));
        };
    }
    std::string serialize() const;
    bool deserialize(const std::string& data);
    Entity create();                       // 新建实体
    void destroy(Entity e);                // 销毁实体及其全部组件
    bool valid(Entity e) const;            // 实体是否存活
    size_t entityCount() const { return alive_.size(); }

    template<typename T>
    T& add(Entity e, T comp = T{}) { return poolFor<T>().data.emplace(e, std::move(comp)).first->second; }
    template<typename T>
    T* get(Entity e) {
        auto ti = std::type_index(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end()) return nullptr;
        auto& d = static_cast<Pool<T>*>(it->second.get())->data;
        auto jt = d.find(e);
        return jt == d.end() ? nullptr : &jt->second;
    }
    template<typename T>
    const T* get(Entity e) const {
        auto ti = std::type_index(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end()) return nullptr;
        const auto& d = static_cast<const Pool<T>*>(it->second.get())->data;
        auto jt = d.find(e);
        return jt == d.end() ? nullptr : &jt->second;
    }
    template<typename T>
    bool has(Entity e) const {
        auto it = pools_.find(std::type_index(typeid(T)));
        return it != pools_.end() && it->second->contains(e);
    }
    template<typename T>
    void remove(Entity e) {
        auto it = pools_.find(std::type_index(typeid(T)));
        if (it != pools_.end()) it->second->erase(e);
    }
    // 遍历同时拥有 Ts... 全部组件的实体: each<Transform,Sprite>([](Entity,Transform&,Sprite&){...})
    template<typename... Ts, typename F>
    void each(F&& fn) {
        static_assert(sizeof...(Ts) > 0, "World::each 至少需要一个组件类型");
        using First = std::tuple_element_t<0, std::tuple<Ts...>>;
        auto ti = std::type_index(typeid(First));
        auto it = pools_.find(ti);
        if (it == pools_.end()) return;
        auto& d = static_cast<Pool<First>*>(it->second.get())->data;
        for (auto& kv : d) {
            Entity e = kv.first;
            if (!valid(e)) continue;
            if ((has<Ts>(e) && ...)) fn(e, *get<Ts>(e)...);
        }
    }
    template<typename T, typename... Args>
    T& addSystem(Args&&... args) {         // 注册系统(按注册顺序每帧执行)
        systems_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return *static_cast<T*>(systems_.back().get());
    }
    void update(float dt);                 // 依次执行全部系统
};

// ============================== 资源与存档 [进阶] ==============================

// [进阶] 资源管理器: 按路径缓存 Texture/Font/AudioClip, 全局单例
class ResourceManager {
    std::unordered_map<std::string, std::unique_ptr<Texture>>   tex_;
    std::unordered_map<std::string, std::unique_ptr<Font>>      font_;
    std::unordered_map<std::string, std::unique_ptr<AudioClip>> clip_;
public:
    static ResourceManager& instance();
    Texture*   texture(const char* path);                    // 加载并缓存(失败返回 nullptr)
    Font*      font(const char* path, float size = 16);      // key = "路径@字号"
    AudioClip* audioClip(const char* path);
    void clear();                                            // 释放全部缓存
};

// [进阶] 存档: 文本/二进制读写 + key=value 键值存档
class SaveSystem {
public:
    static bool writeText(const char* path, const std::string& content);
    static bool readText(const char* path, std::string& out);
    static bool writeBinary(const char* path, const void* data, size_t size);
    static bool readBinary(const char* path, std::vector<unsigned char>& out);
    static bool save(const char* path, const std::unordered_map<std::string, std::string>& kv);
    static bool load(const char* path, std::unordered_map<std::string, std::string>& kv);
    // 键值便捷读取
    static int         getInt(const std::unordered_map<std::string, std::string>& kv, const char* key, int def = 0);
    static float       getFloat(const std::unordered_map<std::string, std::string>& kv, const char* key, float def = 0);
    static std::string getStr(const std::unordered_map<std::string, std::string>& kv, const char* key, const char* def = "");
};

// [扩展] 极简 JSON: 解析/生成, 用于配置与数据驱动
struct Json {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool boolean = false;
    double number = 0;
    std::string str;
    std::vector<Json> arr;
    std::map<std::string, Json> obj;
    bool isNull()   const { return type == Null; }
    bool isArray()  const { return type == Array; }
    bool isObject() const { return type == Object; }
    bool has(const char* key) const { return type == Object && obj.count(key) != 0; }
    const Json& operator[](const char* key) const;      // 不存在时返回静态 Null
    const Json& at(size_t i) const;
    size_t size() const { return type == Array ? arr.size() : obj.size(); }
    double asNumber(double def = 0) const { return type == Number ? number : def; }
    float  asFloat(float def = 0) const { return type == Number ? (float)number : def; }
    int    asInt(int def = 0) const { return type == Number ? (int)number : def; }
    bool   asBool(bool def = false) const { return type == Bool ? boolean : def; }
    std::string asString(const char* def = "") const { return type == String ? str : def; }
    static bool parse(const std::string& text, Json& out, std::string* err = nullptr);
    static bool load(const char* path, Json& out);      // 从文件解析
    std::string dump(int indent = 0) const;             // 序列化
};
using Config = Json;   // 配置即 JSON

// [扩展] 对象池: 预分配固定数量对象, acquire/release 复用(子弹/特效等高频对象)
template<typename T>
class ObjectPool {
    std::vector<std::unique_ptr<T>> objs_;
    std::vector<char> used_;
    std::vector<size_t> free_;
    std::vector<size_t> active_;             // 在用对象下标, forEach 只遍历在用(O(active))
    std::vector<size_t> idxInActive_;         // 对象下标 -> active_ 位置
    std::unordered_map<T*, size_t> index_;   // 指针 -> 下标, release O(1)
    void build(size_t capacity) {
        objs_.reserve(capacity);
        used_.assign(capacity, 0);
        free_.reserve(capacity);
        free_.clear();
        active_.clear();
        idxInActive_.assign(capacity, capacity);
        index_.clear();
        index_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            index_[objs_[i].get()] = i;
            free_.push_back(capacity - 1 - i);
        }
    }
public:
    explicit ObjectPool(size_t capacity = 64) {
        objs_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) objs_.push_back(std::make_unique<T>());
        build(capacity);
    }
    // 带构造参数版本: 池内所有对象用同一组参数构造
    template<typename... Args>
    ObjectPool(size_t capacity, Args&&... args) {
        objs_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i)
            objs_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        build(capacity);
    }
    T* acquire() {                          // 取一个空闲对象(耗尽返回 nullptr)
        if (free_.empty()) return nullptr;
        size_t i = free_.back(); free_.pop_back(); used_[i] = 1;
        idxInActive_[i] = active_.size();
        active_.push_back(i);
        return objs_[i].get();
    }
    void release(T* p) {                    // 归还(注意: 不会自动调用任何清理)
        if (!p) return;
        auto it = index_.find(p);
        if (it != index_.end() && used_[it->second]) {
            const size_t i = it->second;
            const size_t activeIndex = idxInActive_[i];
            const size_t last = active_.back();
            active_[activeIndex] = last;
            idxInActive_[last] = activeIndex;
            active_.pop_back();
            idxInActive_[i] = active_.size();
            used_[i] = 0;
            free_.push_back(i);
        }
    }
    template<typename F>
    void forEach(F&& fn) {                  // 只遍历在用对象(O(active), 不再扫全池)
        for (size_t i : active_) fn(*objs_[i]);
    }
    size_t capacity() const { return objs_.size(); }
    size_t aliveCount() const { return objs_.size() - free_.size(); }
};

// ============================== 玩法: 背包 / 交互 / 对话 / 任务 [进阶] ==============================

// [基础] 物品定义(静态数据, 通常从 Config/数值表加载)
struct Item {
    std::string id;           // 唯一标识(如 "carrot")
    std::string name;         // 显示名
    std::string description;  // 描述
    int maxStack = 99;        // 单格最大堆叠数量
    bool usable = false;      // 是否可被"使用"
    int value = 0;            // 价值(金币等)
};

// [基础] 物品堆: 物品定义 + 当前数量
struct ItemStack {
    Item item;
    int count = 0;
};

// [进阶] 全局物品注册表: 按 id 登记物品定义, 反序列化时按 id 还原完整 Item
std::unordered_map<std::string, Item>& itemRegistry();
void registerItem(const Item& item);
const Item* findItem(const std::string& id);

// [进阶] 背包: 固定容量 + 自动堆叠 + 序列化
class Inventory {
    std::vector<ItemStack> slots_;
    int capacity_ = 0;
public:
    Inventory() = default;
    explicit Inventory(int capacity) { resize(capacity); }
    void resize(int capacity);                   // 重设容量(清空现有内容)
    int capacity() const { return capacity_; }
    const std::vector<ItemStack>& slots() const { return slots_; }
    ItemStack& slot(int i) { return slots_[i]; }
    int add(const Item& item, int count);        // 加入物品, 返回实际加入数量(自动堆叠/填空格)
    int remove(const std::string& id, int count); // 移除物品, 返回实际移除数量
    int count(const std::string& id) const;      // 某物品当前持有总数
    bool has(const std::string& id, int count = 1) const;
    bool isFull() const;                         // 是否已满(无空格且无可堆叠)
    void clear();
    std::string serialize() const;               // 序列化为 JSON 字符串(依赖注册表)
    bool deserialize(const std::string& data);   // 从 JSON 反序列化(按 id 查注册表)
};

// [进阶] 可交互物基类: 玩家站在范围内按交互键触发
class Interactable {
public:
    Vec2 position{0, 0};                 // 触发区域左上角
    Vec2 size{32, 32};                   // 触发区域大小
    std::string id;
    std::string prompt = "按 E 交互";     // 交互提示文本
    bool enabled = true;
    virtual ~Interactable() = default;
    // 玩家碰撞盒是否进入交互范围(默认 AABB 判定)
    virtual bool inRange(Vec2 playerPos, Vec2 playerSize) const;
    virtual void onInteract() = 0;       // 触发交互
};

// [进阶] 拾取物: 交互后把物品加入背包并消失
class PickupInteractable : public Interactable {
public:
    std::string itemId;                  // 拾取的物品 id(查注册表)
    int amount = 1;
    Inventory* inventory = nullptr;      // 目标背包(不拥有, 调用方保证生命周期)
    bool picked = false;                 // 已被拾取
    void onInteract() override;
};

// [进阶] 门: 交互后开关切换
class DoorInteractable : public Interactable {
public:
    bool open = false;
    void onInteract() override { open = !open; }
};

// [基础] 对话节点: 说话人 + 文本 + 选项分支
struct DialogNode {
    std::string speaker;                 // 说话人(空表示旁白)
    std::string text;
    std::vector<std::string> choices;    // 选项文本(空则无分支, 顺序推进)
    std::vector<int> next;               // 每个选项跳转的节点下标(-1 表示结束)
};

// [进阶] 对话树: 推进 / 选项分支 / JSON 加载
class Dialog {
    std::vector<DialogNode> nodes_;
    int current_ = 0;
    bool finished_ = true;
public:
    void clear();
    int addNode(const DialogNode& n);    // 追加节点, 返回下标
    void start() { current_ = 0; finished_ = nodes_.empty(); }
    void startAt(int node);
    const DialogNode* current() const;   // 当前节点(无则 nullptr)
    int currentIndex() const { return current_; }
    bool choose(int option);             // 选择第 option 个选项分支
    void advance();                      // 无分支时推进(链式对话)
    bool finished() const { return finished_; }
    bool load(const Json& data);         // 从 JSON 加载对话树
};

// [基础] 任务: 状态机(未接/进行中/完成)
struct Quest {
    enum class State { NotStarted, InProgress, Completed };
    std::string id;
    std::string title;
    std::string description;
    State state = State::NotStarted;
    std::string objectiveId;             // 追踪的进度键(如 "harvest_carrot")
    int required = 0;                    // 目标需求数量
    int progress = 0;                    // 当前进度
    void start();                        // 接取(NotStarted -> InProgress)
    void advance(int n = 1);             // 推进度(达到 required 自动完成)
    void complete();                     // 直接完成
    void reset();                        // 重置为未接
    bool isCompleted() const { return state == State::Completed; }
};

// ============================== 基础设施 [基础] ==============================

// [基础] 时间(全局静态); 由 App/GameLoop 每帧驱动
class Time {
public:
    static float    delta();        // 本帧时长(秒, 已应用时间缩放; 暂停时为 0)
    static double   total();        // 启动以来总时长(秒)
    static float    fps();          // 平滑后的帧率
    static uint64_t frame();        // 累计帧数
    static void _tick();            // [内部] 每帧调用
    static void setTimeScale(float s);   // [基础] 时间缩放(1=正常, 0.5=慢放, 2=快进)
    static float timeScale();            // [基础] 当前时间缩放
    static void setPaused(bool p);       // [基础] 暂停(暂停时 delta() 返回 0)
    static bool paused();                // [基础] 是否暂停
};

// [扩展] 事件总线(全局单例): 类型安全的发布/订阅, 用于模块解耦
// 用法: auto id = EventBus::instance().subscribe<MyEvent>([](const MyEvent& e){...});
//       EventBus::instance().emit(MyEvent{...});
class EventBus {
    struct Sub { int id; std::function<void(const void*)> fn; };
    std::unordered_map<std::type_index, std::vector<Sub>> subs_;
    std::unordered_map<int, std::type_index> idToType_;   // 退订 O(1) 定位类型
    int nextId_ = 1;
public:
    static EventBus& instance();
    template<typename E>
    int subscribe(std::function<void(const E&)> fn) {
        int id = nextId_++;
        subs_[typeid(E)].push_back({id, [fn](const void* p) { fn(*static_cast<const E*>(p)); }});
        idToType_.emplace(id, typeid(E));
        return id;
    }
    void unsubscribe(int id);
    template<typename E>
    void emit(const E& e) {
        auto it = subs_.find(typeid(E));
        if (it == subs_.end()) return;
        auto list = it->second;   // 注意: 拷贝一份再回调, 防止回调中退订导致迭代失效
        for (auto& s : list) s.fn(&e);
    }
};

// [扩展] 性能剖析: RAII 计时 + 报表; 配合 LGNITE_PROFILE("名") 宏使用
class Profiler {
public:
    struct Stat { double totalMs = 0, maxMs = 0, lastMs = 0, recentMs = 0; long count = 0; std::vector<double> recent; };
    class Scope {                            // 作用域计时, 析构时入账
        Profiler* p_ = nullptr; const char* name_;
        std::chrono::steady_clock::time_point t0_;
    public:
        Scope(Profiler* p, const char* n) : p_(p), name_(n), t0_(std::chrono::steady_clock::now()) {}
        ~Scope();
        Scope(const Scope&) = delete;                    // 禁止拷贝(防止重复入账)
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& o) noexcept : p_(o.p_), name_(o.name_), t0_(o.t0_) { o.p_ = nullptr; }
        Scope& operator=(Scope&&) = delete;
    };
    static Profiler& instance();
    Scope scope(const char* name) { return Scope(this, name); }
    void reset();
    const std::unordered_map<std::string, Stat>& stats() const { return stats_; }
    void print();                            // 通过 Log 打印统计表
    void _add(const char* name, double ms);  // [内部]
private:
    std::unordered_map<std::string, Stat> stats_;
};
#define LGNITE_DETAIL_CONCAT(a, b) a##b
#define LGNITE_CONCAT(a, b) LGNITE_DETAIL_CONCAT(a, b)
// 在函数/代码块内放一行即可统计耗时
#define LGNITE_PROFILE(name) auto LGNITE_CONCAT(_lgnite_prof_, __LINE__) = ::lgnite::Profiler::instance().scope(name)

// [基础] 游戏主循环: 固定步长更新 + 每帧渲染(手动管理窗口时使用)
class GameLoop {
    bool running_ = true;
public:
    using UpdateFn = std::function<void(float)>;
    using RenderFn = std::function<void()>;
    // 运行直到窗口关闭或 stop(); fps 为逻辑更新频率(渲染每帧一次)
    int run(Window& win, UpdateFn update, RenderFn render, int fps = 60);
    void stop();
};

// [基础] 一站式启动器: 封装"开窗 + 初始化 + 主循环 + 清理", 10 行出画面
struct App {
    int width = 800, height = 600;
    const char* title = "lgnite app";
    bool vsync = true;
    int targetFps = 60;                       // [基础] <=0 不限帧
    bool audio = true;                         // 是否自动初始化音频
    Color clearColor{0.1f, 0.1f, 0.12f, 1};    // 清屏色
    std::function<void()> setup;               // 启动后调用一次
    std::function<void(float)> update;         // 每帧逻辑(dt 秒)
    std::function<void()> draw;                // 每帧渲染
    int run();                                 // 返回进程退出码
    Window* window() const { return win_; }    // 运行期间访问底层窗口(改标题/全屏等)
private:
    Window* win_ = nullptr;                    // [内部] run() 期间有效
};

// ============================== 工具函数 [基础] ==============================

float clampf(float v, float lo, float hi);
float lerpf(float a, float b, float t);
float randf(float lo, float hi);   // [lo, hi) 随机浮点
int   randi(int lo, int hi);       // [lo, hi] 随机整数
constexpr float PI = 3.14159265358979f;
inline float deg2rad(float deg) { return deg * PI / 180.f; }

} // namespace lgnite

// ############################################################################
// ########################### 实现区(IMPLEMENTATION) #########################
// ############################################################################
#ifdef LGNITE_IMPLEMENTATION

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

// ---- 第三方单头实现(这些宏只能出现在一个翻译单元, 因此必须放在本宏内) ----
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include "stb_truetype.h"
#ifndef MINIAUDIO_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#endif
#include "miniaudio.h"

namespace lgnite {

// ============================== OpenGL 最小声明与加载器 ==============================
// 说明: 不依赖 GLAD/GLEW, 仅补齐本库用到的类型/常量/函数指针;
//       新增 GL 函数时在 LGNITE_GL_LIST 里登记一行即可。

#if defined(_WIN32)
#   ifndef APIENTRY
#   define APIENTRY __stdcall
#   endif
#else
#   ifndef APIENTRY
#   define APIENTRY
#   endif
#endif

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef char           GLchar;
typedef ptrdiff_t      GLsizeiptr;
typedef ptrdiff_t      GLintptr;

// ---- 常量补齐(仅本库用到的) ----
#define LGNITE_GL_FALSE                 0
#define LGNITE_GL_TRUE                  1
#define LGNITE_GL_NO_ERROR              0
#define LGNITE_GL_TRIANGLES             0x0004
#define LGNITE_GL_COLOR_BUFFER_BIT      0x00004000
#define LGNITE_GL_DEPTH_BUFFER_BIT      0x00000100
#define LGNITE_GL_BLEND                 0x0BE2
#define LGNITE_GL_DEPTH_TEST            0x0B71
#define LGNITE_GL_SRC_ALPHA             0x0302
#define LGNITE_GL_ONE                    1
#define LGNITE_GL_ONE_MINUS_SRC_ALPHA   0x0303
#define LGNITE_GL_UNSIGNED_BYTE         0x1401
#define LGNITE_GL_UNSIGNED_INT          0x1405
#define LGNITE_GL_FLOAT                 0x1406
#define LGNITE_GL_RGB                   0x1907
#define LGNITE_GL_RGBA                  0x1908
#define LGNITE_GL_UNPACK_ALIGNMENT      0x0CF5
#define LGNITE_GL_TEXTURE_2D            0x0DE1
#define LGNITE_GL_TEXTURE0              0x84C0
#define LGNITE_GL_TEXTURE_WRAP_S        0x2802
#define LGNITE_GL_TEXTURE_WRAP_T        0x2803
#define LGNITE_GL_TEXTURE_MIN_FILTER    0x2801
#define LGNITE_GL_TEXTURE_MAG_FILTER    0x2800
#define LGNITE_GL_NEAREST               0x2600
#define LGNITE_GL_LINEAR                0x2601
#define LGNITE_GL_CLAMP_TO_EDGE         0x812F
#define LGNITE_GL_REPEAT                0x2901
#define LGNITE_GL_ARRAY_BUFFER          0x8892
#define LGNITE_GL_ELEMENT_ARRAY_BUFFER  0x8893
#define LGNITE_GL_STATIC_DRAW           0x88E4
#define LGNITE_GL_DYNAMIC_DRAW          0x88E8
#define LGNITE_GL_FRAGMENT_SHADER       0x8B30
#define LGNITE_GL_VERTEX_SHADER         0x8B31
#define LGNITE_GL_COMPILE_STATUS        0x8B81
#define LGNITE_GL_LINK_STATUS           0x8B82
#define LGNITE_GL_INFO_LOG_LENGTH       0x8B84
#define LGNITE_GL_VENDOR                0x1F00
#define LGNITE_GL_RENDERER              0x1F01
#define LGNITE_GL_VERSION               0x1F02
#define LGNITE_GL_INVALID_ENUM          0x0500
#define LGNITE_GL_INVALID_VALUE         0x0501
#define LGNITE_GL_INVALID_OPERATION     0x0502
#define LGNITE_GL_OUT_OF_MEMORY         0x0505
#define LGNITE_GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

// ---- 函数指针表: 新增 GL 函数在此登记 ----
#define LGNITE_GL_LIST(X) \
    X(void,    glViewport,              GLint, GLint, GLsizei, GLsizei) \
    X(void,    glClearColor,            GLfloat, GLfloat, GLfloat, GLfloat) \
    X(void,    glClear,                 GLbitfield) \
    X(void,    glEnable,                GLenum) \
    X(void,    glDisable,               GLenum) \
    X(void,    glBlendFunc,             GLenum, GLenum) \
    X(GLenum,  glGetError,              void) \
    X(const GLubyte*, glGetString,      GLenum) \
    X(void,    glGenVertexArrays,       GLsizei, GLuint*) \
    X(void,    glBindVertexArray,       GLuint) \
    X(void,    glDeleteVertexArrays,    GLsizei, const GLuint*) \
    X(void,    glGenBuffers,            GLsizei, GLuint*) \
    X(void,    glBindBuffer,            GLenum, GLuint) \
    X(void,    glBufferData,            GLenum, GLsizeiptr, const void*, GLenum) \
    X(void,    glBufferSubData,         GLenum, GLintptr, GLsizeiptr, const void*) \
    X(void,    glDeleteBuffers,         GLsizei, const GLuint*) \
    X(void,    glVertexAttribPointer,   GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) \
    X(void,    glEnableVertexAttribArray, GLuint) \
    X(GLuint,  glCreateShader,          GLenum) \
    X(void,    glShaderSource,          GLuint, GLsizei, const GLchar* const*, const GLint*) \
    X(void,    glCompileShader,         GLuint) \
    X(void,    glGetShaderiv,           GLuint, GLenum, GLint*) \
    X(void,    glGetShaderInfoLog,      GLuint, GLsizei, GLsizei*, GLchar*) \
    X(void,    glDeleteShader,          GLuint) \
    X(GLuint,  glCreateProgram,         void) \
    X(void,    glAttachShader,          GLuint, GLuint) \
    X(void,    glLinkProgram,           GLuint) \
    X(void,    glGetProgramiv,          GLuint, GLenum, GLint*) \
    X(void,    glGetProgramInfoLog,     GLuint, GLsizei, GLsizei*, GLchar*) \
    X(void,    glDeleteProgram,         GLuint) \
    X(void,    glUseProgram,            GLuint) \
    X(GLint,   glGetUniformLocation,    GLuint, const GLchar*) \
    X(void,    glUniform1i,             GLint, GLint) \
    X(void,    glUniform1f,             GLint, GLfloat) \
    X(void,    glUniform2f,             GLint, GLfloat, GLfloat) \
    X(void,    glUniform4f,             GLint, GLfloat, GLfloat, GLfloat, GLfloat) \
    X(void,    glUniformMatrix4fv,      GLint, GLsizei, GLboolean, const GLfloat*) \
    X(void,    glGenTextures,           GLsizei, GLuint*) \
    X(void,    glBindTexture,           GLenum, GLuint) \
    X(void,    glDeleteTextures,        GLsizei, const GLuint*) \
    X(void,    glTexImage2D,            GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) \
    X(void,    glTexSubImage2D,         GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*) \
    X(void,    glTexParameteri,         GLenum, GLenum, GLint) \
    X(void,    glActiveTexture,         GLenum) \
    X(void,    glDrawElements,          GLenum, GLsizei, GLenum, const void*) \
    X(void,    glDrawArrays,            GLenum, GLint, GLsizei) \
    X(void,    glPixelStorei,           GLenum, GLint) \
    X(void,    glLineWidth,             GLfloat) \
    X(void,    glGenFramebuffers,         GLsizei, GLuint*) \
    X(void,    glBindFramebuffer,         GLenum, GLuint) \
    X(void,    glFramebufferTexture2D,    GLenum, GLenum, GLenum, GLuint, GLint) \
    X(GLenum,  glCheckFramebufferStatus,  GLenum) \
    X(void,    glDeleteFramebuffers,      GLsizei, const GLuint*)

#define LGNITE_GL_TYPEDEF(ret, name, ...) typedef ret (APIENTRY* LGNITE_PFN_##name)(__VA_ARGS__);
LGNITE_GL_LIST(LGNITE_GL_TYPEDEF)
#undef LGNITE_GL_TYPEDEF

#define LGNITE_GL_DECL(ret, name, ...) static LGNITE_PFN_##name name = nullptr;
LGNITE_GL_LIST(LGNITE_GL_DECL)
#undef LGNITE_GL_DECL

static bool lgnite_gl_load() {
    bool ok = true;
#define LGNITE_GL_LOAD(ret, name, ...) \
    name = (LGNITE_PFN_##name)glfwGetProcAddress(#name); \
    if (!name) { Log::error("[gl] 函数加载失败: %s", #name); ok = false; }
    LGNITE_GL_LIST(LGNITE_GL_LOAD)
#undef LGNITE_GL_LOAD
    return ok;
}

// ============================== 内部辅助(detail) ==============================
namespace detail {

bool readFileBytes(const char* path, std::vector<unsigned char>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize((size_t)n);
    return n == 0 || f.read((char*)out.data(), n).good();
}

// 编译着色器, 失败写日志并返回 0
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, LGNITE_GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024]; GLsizei len = 0;
        glGetShaderInfoLog(s, sizeof(buf), &len, buf);
        Log::error("[Shader] 编译失败: %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, LGNITE_GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024]; GLsizei len = 0;
        glGetProgramInfoLog(p, sizeof(buf), &len, buf);
        Log::error("[Shader] 链接失败: %s", buf);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace detail

// ============================== Mat4 ==============================
Mat4 Mat4::identity() {
    Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.f;
    return r;
}
Mat4 Mat4::ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 m = identity();
    m.m[0]  = 2.f / (r - l);
    m.m[5]  = 2.f / (t - b);
    m.m[10] = -2.f / (f - n);
    m.m[12] = -(r + l) / (r - l);
    m.m[13] = -(t + b) / (t - b);
    m.m[14] = -(f + n) / (f - n);
    return m;
}
Mat4 Mat4::translation(float x, float y) {
    Mat4 m = identity();
    m.m[12] = x; m.m[13] = y;
    return m;
}
Mat4 Mat4::scaling(float x, float y) {
    Mat4 m = identity();
    m.m[0] = x; m.m[5] = y;
    return m;
}
Mat4 Mat4::rotationZ(float deg) {
    Mat4 m = identity();
    float r = deg2rad(deg), c = std::cos(r), s = std::sin(r);
    m.m[0] = c; m.m[1] = s; m.m[4] = -s; m.m[5] = c;
    return m;
}
Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += m[k * 4 + row] * o.m[c * 4 + k];
            r.m[c * 4 + row] = s;
        }
    return r;
}
Mat4 Transform::matrix() const {
    return Mat4::translation(pos.x, pos.y) * Mat4::rotationZ(rotation) * Mat4::scaling(scale.x, scale.y);
}

// ============================== 基础类型 ==============================
Vec2 Vec2::lerp(Vec2 a, Vec2 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }
float Vec2::angle() const { return std::atan2(y, x) * 180.f / PI; }
Vec2 Vec2::rotated(float deg) const {
    float r = deg2rad(deg), c = std::cos(r), s = std::sin(r);
    return {x * c - y * s, x * s + y * c};
}
float Vec2::dist(Vec2 a, Vec2 b) { return (a - b).length(); }

bool Rect::contains(Vec2 p) const { return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h; }
bool pointInRect(Vec2 p, const Rect& r) { return r.contains(p); }
bool Rect::intersects(const Rect& o) const {
    return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y;
}
Rect Rect::inflated(float d) const { return {x - d, y - d, w + 2 * d, h + 2 * d}; }
Rect Rect::offset(float dx, float dy) const { return {x + dx, y + dy, w, h}; }
Rect Rect::scaled(float k) const {
    Vec2 c = center();
    float nw = w * k, nh = h * k;
    return {c.x - nw * 0.5f, c.y - nh * 0.5f, nw, nh};
}

Color Color::RGBA255(int r, int g, int b, int a) {
    return {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
}
Color Color::Hex(uint32_t rgb, float a) {
    return RGBA255((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, (int)(a * 255));
}
Color Color::lerp(Color c0, Color c1, float t) {
    return {c0.r + (c1.r - c0.r) * t, c0.g + (c1.g - c0.g) * t,
            c0.b + (c1.b - c0.b) * t, c0.a + (c1.a - c0.a) * t};
}
Color Color::HSV(float h, float s, float v, float a) {
    h = std::fmod(h, 360.f); if (h < 0) h += 360.f;
    s = clampf(s, 0, 1); v = clampf(v, 0, 1);
    float c = v * s, x = c * (1 - std::fabs(std::fmod(h / 60.f, 2.f) - 1)), m = v - c;
    float r = 0, g = 0, b = 0;
    if (h < 60) { r = c; g = x; } else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; } else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; } else { r = c; b = x; }
    return {r + m, g + m, b + m, a};
}
Color Color::HSL(float h, float s, float l, float a) {
    h = std::fmod(h, 360.f); if (h < 0) h += 360.f;
    s = clampf(s, 0, 1); l = clampf(l, 0, 1);
    float v = l + s * std::min(l, 1 - l);
    float sv = v == 0 ? 0 : 2 * (1 - l / v);
    return HSV(h, sv, v, a);
}
void Color::toHSV(float& h, float& s, float& v) const {
    float mx = std::max(r, std::max(g, b)), mn = std::min(r, std::min(g, b)), d = mx - mn;
    v = mx; s = mx <= 0 ? 0 : d / mx;
    if (d == 0) h = 0;
    else if (mx == r) h = 60.f * std::fmod((g - b) / d, 6.f);
    else if (mx == g) h = 60.f * ((b - r) / d + 2.f);
    else h = 60.f * ((r - g) / d + 4.f);
    if (h < 0) h += 360.f;
}
void Color::toHSL(float& h, float& s, float& l) const {
    float mx = std::max(r, std::max(g, b)), mn = std::min(r, std::min(g, b)), d = mx - mn;
    l = (mx + mn) * 0.5f;
    if (d == 0) { h = 0; s = 0; return; }
    s = d / (1 - std::fabs(2 * l - 1));
    if (mx == r) h = 60.f * std::fmod((g - b) / d, 6.f);
    else if (mx == g) h = 60.f * ((b - r) / d + 2.f);
    else h = 60.f * ((r - g) / d + 4.f);
    if (h < 0) h += 360.f;
}

// ============================== Log ==============================
namespace {
Log::Sink& logSink() { static Log::Sink s; return s; }
void logV(Log::Level lv, const char* fmt, va_list ap) {
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    if (logSink()) { logSink()(lv, buf); return; }
    const char* tag = lv == Log::Info ? "INFO" : lv == Log::Warn ? "WARN" : "ERROR";
    std::fprintf(lv == Log::Error ? stderr : stdout, "[lgnite][%s] %s\n", tag, buf);
}
}
void Log::setSink(Sink s) { logSink() = std::move(s); }
void Log::info(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); logV(Info, fmt, ap);  va_end(ap); }
void Log::warn(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); logV(Warn, fmt, ap);  va_end(ap); }
void Log::error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); logV(Error, fmt, ap); va_end(ap); }

// ============================== Time ==============================
namespace {
struct TimeState {
    float dt = 0; double total = 0; float fps = 0; uint64_t frame = 0;
    std::chrono::steady_clock::time_point last;
    bool started = false;
    float scale = 1; bool paused = false;
};
TimeState& ts() { static TimeState s; return s; }
}
void Time::_tick() {
    auto& s = ts();
    auto now = std::chrono::steady_clock::now();
    if (!s.started) { s.last = now; s.started = true; return; }
    float dt = std::chrono::duration<float>(now - s.last).count();
    s.last = now;
    if (dt > 0.25f) dt = 0.25f;          // 防止卡顿后 dt 爆炸
    s.dt = dt; s.total += dt; s.frame++;
    float cur = dt > 0 ? 1.f / dt : 0;
    s.fps = s.fps == 0 ? cur : s.fps * 0.95f + cur * 0.05f;   // 平滑
}
float    Time::delta() { auto& s = ts(); return s.paused ? 0.f : s.dt * s.scale; }
double   Time::total() { return ts().total; }
float    Time::fps()   { return ts().fps; }
uint64_t Time::frame() { return ts().frame; }
void Time::setTimeScale(float s) { ts().scale = s > 0 ? s : 1.f; }
float  Time::timeScale() { return ts().scale; }
void Time::setPaused(bool p) { ts().paused = p; }
bool  Time::paused() { return ts().paused; }

// ============================== gl 命名空间 ==============================
namespace gl {
namespace { bool& readyFlag() { static bool f = false; return f; } }
bool init() {
    if (!lgnite_gl_load()) {
        Log::error("[gl] 部分 GL 函数加载失败(注意: 必须先 Window::create 再 gl::init; 失败后请勿继续渲染调用)");
        readyFlag() = false;
        return false;
    }
    readyFlag() = true;
    glEnable(LGNITE_GL_BLEND);
    glBlendFunc(LGNITE_GL_SRC_ALPHA, LGNITE_GL_ONE_MINUS_SRC_ALPHA);
    glDisable(LGNITE_GL_DEPTH_TEST);      // 2D 不需要深度测试
    glPixelStorei(LGNITE_GL_UNPACK_ALIGNMENT, 1);
    Log::info("[gl] OpenGL %s | %s", (const char*)glGetString(LGNITE_GL_VERSION),
              (const char*)glGetString(LGNITE_GL_RENDERER));
    return true;
}
void checkError(const char* tag) {
    GLenum e;
    while ((e = glGetError()) != LGNITE_GL_NO_ERROR) {
        const char* name = "UNKNOWN";
        switch (e) {
            case LGNITE_GL_INVALID_ENUM:      name = "INVALID_ENUM"; break;
            case LGNITE_GL_INVALID_VALUE:     name = "INVALID_VALUE"; break;
            case LGNITE_GL_INVALID_OPERATION: name = "INVALID_OPERATION"; break;
            case LGNITE_GL_OUT_OF_MEMORY:     name = "OUT_OF_MEMORY"; break;
            case LGNITE_GL_INVALID_FRAMEBUFFER_OPERATION: name = "INVALID_FBO"; break;
            default: break;
        }
        Log::error("[gl] %s: 0x%04X %s", tag, e, name);
    }
}
const char* version() {
    const GLubyte* v = glGetString(LGNITE_GL_VERSION);
    return v ? (const char*)v : "unknown";
}
bool ready() { return readyFlag(); }
} // namespace gl

// ============================== Window ==============================
Window::~Window() { destroy(); }

bool Window::create(int w, int h, const char* title, bool vsync) {
    // 注意: glfwInit 可重入(重复调用安全), 直接每次调用, 保证 glfwTerminate 后重建窗口仍可用
    if (!glfwInit()) { Log::error("[Window] GLFW 初始化失败"); return false; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);   // macOS 必须
#endif
    handle_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!handle_) {
        Log::error("[Window] 窗口创建失败(需要支持 OpenGL 3.3 的显卡/驱动)");
        return false;
    }
    w_ = w; h_ = h;
    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(vsync ? 1 : 0);
    return true;
}
void Window::destroy() {
    if (handle_) { glfwDestroyWindow(handle_); handle_ = nullptr; }
}
bool Window::shouldClose() const { return handle_ && glfwWindowShouldClose(handle_); }
void Window::close() { if (handle_) glfwSetWindowShouldClose(handle_, GLFW_TRUE); }
void Window::pollEvents() { glfwPollEvents(); }
void Window::swapBuffers() { if (handle_) glfwSwapBuffers(handle_); }
void Window::setSize(int w, int h) { if (handle_) { glfwSetWindowSize(handle_, w, h); w_ = w; h_ = h; } }
void Window::setTitle(const char* title) { if (handle_) glfwSetWindowTitle(handle_, title); }
bool Window::isFocused() const { return handle_ && glfwGetWindowAttrib(handle_, GLFW_FOCUSED); }
void Window::setFullscreen(bool fs) {
    if (!handle_ || fs == fullscreen_) return;
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    if (!mon || !mode) return;
    if (fs) {
        glfwGetWindowPos(handle_, &savedX_, &savedY_);
        glfwGetWindowSize(handle_, &savedW_, &savedH_);
        glfwSetWindowMonitor(handle_, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(handle_, nullptr, savedX_, savedY_, savedW_, savedH_, 0);
    }
    fullscreen_ = fs;
    glfwGetWindowSize(handle_, &w_, &h_);
}
bool Window::setIcon(const char* path) {
    if (!handle_) return false;
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load(path, &w, &h, &ch, 4);
    if (!px) { Log::error("[Window] 图标加载失败: %s", path); return false; }
    GLFWimage img{w, h, px};
    glfwSetWindowIcon(handle_, 1, &img);
    stbi_image_free(px);
    return true;
}
float Window::getDpiScale() const {
    if (!handle_) return 1.f;
    float xs = 1, ys = 1;
    glfwGetWindowContentScale(handle_, &xs, &ys);
    return xs;
}

// ============================== Texture ==============================
Texture::~Texture() { destroy(); }
Texture::Texture(Texture&& o) noexcept : id_(o.id_), w_(o.w_), h_(o.h_) { o.id_ = 0; o.w_ = o.h_ = 0; }
Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) { destroy(); id_ = o.id_; w_ = o.w_; h_ = o.h_; o.id_ = 0; o.w_ = o.h_ = 0; }
    return *this;
}
bool Texture::create(int w, int h, const unsigned char* rgba, bool repeat) {
    if (w <= 0 || h <= 0 || !rgba) { Log::error("[Texture] 无效像素数据"); return false; }
    destroy();
    glGenTextures(1, &id_);
    glBindTexture(LGNITE_GL_TEXTURE_2D, id_);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MIN_FILTER, LGNITE_GL_LINEAR);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MAG_FILTER, LGNITE_GL_LINEAR);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_WRAP_S, repeat ? LGNITE_GL_REPEAT : LGNITE_GL_CLAMP_TO_EDGE);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_WRAP_T, repeat ? LGNITE_GL_REPEAT : LGNITE_GL_CLAMP_TO_EDGE);
    glTexImage2D(LGNITE_GL_TEXTURE_2D, 0, LGNITE_GL_RGBA, w, h, 0, LGNITE_GL_RGBA, LGNITE_GL_UNSIGNED_BYTE, rgba);
    glBindTexture(LGNITE_GL_TEXTURE_2D, 0);
    w_ = w; h_ = h;
    return true;
}
bool Texture::load(const char* path, bool flipY) {
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load(path, &w, &h, &ch, 4);   // 统一转 RGBA
    if (!px) { Log::error("[Texture] 加载失败: %s (%s)", path, stbi_failure_reason()); return false; }
    bool ok = create(w, h, px);
    stbi_image_free(px);
    return ok;
}
bool Texture::loadFromMemory(const unsigned char* data, int len) {
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load_from_memory(data, len, &w, &h, &ch, 4);
    if (!px) { Log::error("[Texture] 内存解码失败: %s", stbi_failure_reason()); return false; }
    bool ok = create(w, h, px);
    stbi_image_free(px);
    return ok;
}
void Texture::destroy() {
    // 注意: 上下文已销毁(如全局对象在 glfwTerminate 后析构)时跳过 GL 调用, 防止崩溃
    if (id_) {
        if (glfwGetCurrentContext()) glDeleteTextures(1, &id_);
        id_ = 0; w_ = h_ = 0;
    }
}
void Texture::setFilter(bool smooth) {
    if (!id_ || !glfwGetCurrentContext()) return;   // 上下文销毁后调用是空操作, 防崩
    glBindTexture(LGNITE_GL_TEXTURE_2D, id_);
    GLint f = smooth ? LGNITE_GL_LINEAR : LGNITE_GL_NEAREST;
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MIN_FILTER, f);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MAG_FILTER, f);
    glBindTexture(LGNITE_GL_TEXTURE_2D, 0);
}

// ============================== Renderer ==============================
namespace detail {

struct RVertex { float x, y, u, v, r, g, b, a; };
constexpr int R_MAX_QUADS = 8192;

const char* R_VS = R"(#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
uniform mat4 uProj;
out vec2 vUV; out vec4 vColor;
void main(){ vUV=aUV; vColor=aColor; gl_Position = uProj * vec4(aPos,0.0,1.0); }
)";

const char* R_FS = R"(#version 330 core
in vec2 vUV; in vec4 vColor;
uniform sampler2D uTex;
out vec4 FragColor;
void main(){ FragColor = texture(uTex, vUV) * vColor; }
)";

struct RenderState {
    GLuint vao = 0, vbo = 0, ibo = 0, prog = 0, white = 0;
    GLint locProj = -1, locTex = -1;
    std::vector<RVertex> verts;
    std::vector<uint32_t> idx;
    GLuint curTex = 0;                    // 0 = 内置白纹理
    int vw = 800, vh = 600;               // 逻辑分辨率(与视口一致)
    const Camera2D* cam = nullptr;
    bool inited = false;
    bool additive = false;
    int drawCalls = 0;
};
RenderState& RS() { static RenderState s; return s; }

// 提交一个四边形(2 个三角形), 纹理切换或缓冲满时自动 flush
void pushQuad(GLuint tex, const RVertex v[4]) {
    RenderState& rs = RS();
    if (tex != rs.curTex) { Renderer::flush(); rs.curTex = tex; }
    if (rs.idx.size() + 6 > (size_t)R_MAX_QUADS * 6) Renderer::flush();
    uint32_t base = (uint32_t)rs.verts.size();
    rs.verts.insert(rs.verts.end(), v, v + 4);
    uint32_t ii[6] = {base, base + 1, base + 2, base, base + 2, base + 3};
    rs.idx.insert(rs.idx.end(), ii, ii + 6);
}

// 提交一个独立三角形(圆/扇形等用), 与四边形共用动态索引缓冲
void pushTri(GLuint tex, const RVertex& a, const RVertex& b, const RVertex& c) {
    RenderState& rs = RS();
    if (tex != rs.curTex) { Renderer::flush(); rs.curTex = tex; }
    if (rs.idx.size() + 3 > (size_t)R_MAX_QUADS * 6) Renderer::flush();
    uint32_t base = (uint32_t)rs.verts.size();
    rs.verts.push_back(a); rs.verts.push_back(b); rs.verts.push_back(c);
    rs.idx.push_back(base); rs.idx.push_back(base + 1); rs.idx.push_back(base + 2);
}

// 计算带旋转的四个角点(绕中心), 输出按 LT,RT,RB,LB 顺序
void rotatedCorners(float x, float y, float w, float h, float rotDeg, Vec2 out[4]) {
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    out[0] = {x, y}; out[1] = {x + w, y}; out[2] = {x + w, y + h}; out[3] = {x, y + h};
    if (rotDeg != 0) {
        float r = deg2rad(rotDeg), c = std::cos(r), s = std::sin(r);
        for (int i = 0; i < 4; ++i) {
            float dx = out[i].x - cx, dy = out[i].y - cy;
            out[i] = {cx + dx * c - dy * s, cy + dx * s + dy * c};
        }
    }
}

void pushRectVerts(GLuint tex, float x, float y, float w, float h,
                   const Color& c, float rotDeg, float u0, float v0, float u1, float v1) {
    Vec2 p[4];
    rotatedCorners(x, y, w, h, rotDeg, p);
    RVertex v[4] = {
        {p[0].x, p[0].y, u0, v0, c.r, c.g, c.b, c.a},
        {p[1].x, p[1].y, u1, v0, c.r, c.g, c.b, c.a},
        {p[2].x, p[2].y, u1, v1, c.r, c.g, c.b, c.a},
        {p[3].x, p[3].y, u0, v1, c.r, c.g, c.b, c.a},
    };
    pushQuad(tex, v);
}

void pushGradientRect(float x, float y, float w, float h, Color tl, Color tr, Color bl, Color br, float rotDeg) {
    Vec2 p[4]; rotatedCorners(x, y, w, h, rotDeg, p);
    RVertex v[4] = {
        {p[0].x, p[0].y, 0, 0, tl.r, tl.g, tl.b, tl.a},
        {p[1].x, p[1].y, 1, 0, tr.r, tr.g, tr.b, tr.a},
        {p[2].x, p[2].y, 1, 1, br.r, br.g, br.b, br.a},
        {p[3].x, p[3].y, 0, 1, bl.r, bl.g, bl.b, bl.a}
    };
    pushQuad(0, v);
}

void pushRectAtAnchor(GLuint tex, float x, float y, float w, float h, const Color& c,
                      float rotDeg, float ax, float ay, float u0, float v0, float u1, float v1) {
    Vec2 p[4] = {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
    if (rotDeg != 0) {
        float cx = x + w * ax, cy = y + h * ay;
        float r = deg2rad(rotDeg), cs = std::cos(r), sn = std::sin(r);
        for (auto& q : p) {
            float dx = q.x - cx, dy = q.y - cy;
            q = {cx + dx * cs - dy * sn, cy + dx * sn + dy * cs};
        }
    }
    RVertex v[4] = {
        {p[0].x,p[0].y,u0,v0,c.r,c.g,c.b,c.a}, {p[1].x,p[1].y,u1,v0,c.r,c.g,c.b,c.a},
        {p[2].x,p[2].y,u1,v1,c.r,c.g,c.b,c.a}, {p[3].x,p[3].y,u0,v1,c.r,c.g,c.b,c.a}
    };
    pushQuad(tex, v);
}

} // namespace detail

bool Renderer::init() {
    auto& rs = detail::RS();
    if (rs.inited) return true;
    if (!gl::ready()) {   // GL 未就绪时拒绝初始化, 避免后续空指针崩溃
        Log::error("[Renderer] GL 未就绪, 请先成功执行 gl::init()");
        return false;
    }
    GLuint vs = detail::compileShader(LGNITE_GL_VERTEX_SHADER, detail::R_VS);
    GLuint fs = detail::compileShader(LGNITE_GL_FRAGMENT_SHADER, detail::R_FS);
    if (!vs || !fs) return false;
    rs.prog = detail::linkProgram(vs, fs);
    glDeleteShader(vs); glDeleteShader(fs);
    if (!rs.prog) return false;
    rs.locProj = glGetUniformLocation(rs.prog, "uProj");
    rs.locTex  = glGetUniformLocation(rs.prog, "uTex");

    rs.verts.reserve(detail::R_MAX_QUADS * 4);
    rs.idx.reserve(detail::R_MAX_QUADS * 6);

    glGenVertexArrays(1, &rs.vao);
    glBindVertexArray(rs.vao);
    glGenBuffers(1, &rs.vbo);
    glBindBuffer(LGNITE_GL_ARRAY_BUFFER, rs.vbo);
    glBufferData(LGNITE_GL_ARRAY_BUFFER, detail::R_MAX_QUADS * 4 * sizeof(detail::RVertex), nullptr, LGNITE_GL_DYNAMIC_DRAW);
    // 索引缓冲改为动态上传(支持三角形/四边形混合批次)
    glGenBuffers(1, &rs.ibo);
    glBindBuffer(LGNITE_GL_ELEMENT_ARRAY_BUFFER, rs.ibo);
    glBufferData(LGNITE_GL_ELEMENT_ARRAY_BUFFER, detail::R_MAX_QUADS * 6 * sizeof(uint32_t), nullptr, LGNITE_GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, LGNITE_GL_FLOAT, LGNITE_GL_FALSE, sizeof(detail::RVertex), (void*)offsetof(detail::RVertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, LGNITE_GL_FLOAT, LGNITE_GL_FALSE, sizeof(detail::RVertex), (void*)offsetof(detail::RVertex, u));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, LGNITE_GL_FLOAT, LGNITE_GL_FALSE, sizeof(detail::RVertex), (void*)offsetof(detail::RVertex, r));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    unsigned char white[4] = {255, 255, 255, 255};   // 1x1 白纹理: 图形绘制共用同一管线
    glGenTextures(1, &rs.white);
    glBindTexture(LGNITE_GL_TEXTURE_2D, rs.white);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MIN_FILTER, LGNITE_GL_NEAREST);
    glTexParameteri(LGNITE_GL_TEXTURE_2D, LGNITE_GL_TEXTURE_MAG_FILTER, LGNITE_GL_NEAREST);
    glTexImage2D(LGNITE_GL_TEXTURE_2D, 0, LGNITE_GL_RGBA, 1, 1, 0, LGNITE_GL_RGBA, LGNITE_GL_UNSIGNED_BYTE, white);
    glBindTexture(LGNITE_GL_TEXTURE_2D, 0);

    rs.inited = true;
    return true;
}
void Renderer::shutdown() {
    auto& rs = detail::RS();
    if (!rs.inited) return;
    if (glfwGetCurrentContext()) {          // 上下文存活时才做 GL 清理
        glDeleteBuffers(1, &rs.vbo);
        glDeleteBuffers(1, &rs.ibo);
        glDeleteVertexArrays(1, &rs.vao);
        glDeleteProgram(rs.prog);
        glDeleteTextures(1, &rs.white);
    }
    rs = detail::RenderState();
}
void Renderer::clear(const Color& c) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(LGNITE_GL_COLOR_BUFFER_BIT);
}
void Renderer::setViewport(int w, int h) {
    auto& rs = detail::RS();
    rs.vw = w; rs.vh = h;
    glViewport(0, 0, w, h);
}
void Renderer::setCamera(const Camera2D* cam) {
    if (detail::RS().cam != cam) {  
        Renderer::flush();
    }
    detail::RS().cam = cam;
}
void Renderer::drawRect(float x, float y, float w, float h, const Color& c, float rotDeg) {
    detail::pushRectVerts(0, x, y, w, h, c, rotDeg, 0, 0, 1, 1);
}
void Renderer::drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& c, Renderer::LineCap cap) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0 || thickness <= 0) {
        if (len == 0 && cap == LineCap::Round && thickness > 0) drawCircle(x1, y1, thickness * 0.5f, c, 24);
        return;
    }
    float deg = std::atan2(dy, dx) * 180.f / PI;
    if (cap == LineCap::Square) {
        float ex = dx / len * thickness * 0.5f, ey = dy / len * thickness * 0.5f;
        x1 -= ex; y1 -= ey; x2 += ex; y2 += ey;
        dx = x2 - x1; dy = y2 - y1; len = std::sqrt(dx * dx + dy * dy);
        deg = std::atan2(dy, dx) * 180.f / PI;
    }
    // 以线段中心为轴画一个旋转矩形
    float cx = (x1 + x2) * 0.5f, cy = (y1 + y2) * 0.5f;
    detail::pushRectVerts(0, cx - len * 0.5f, cy - thickness * 0.5f, len, thickness, c, deg, 0, 0, 1, 1);
    if (cap == LineCap::Round) {
        drawCircle(x1, y1, thickness * 0.5f, c, 20);
        drawCircle(x2, y2, thickness * 0.5f, c, 20);
    }
}
void Renderer::drawCircle(float cx, float cy, float radius, const Color& c, int segments) {
    if (radius <= 0) return;
    if (segments < 3) segments = 3;
    // 真三角扇: segments 个三角形共享圆心, 无缺块无缝隙
    detail::RVertex center{cx, cy, 0.5f, 0.5f, c.r, c.g, c.b, c.a};
    for (int i = 0; i < segments; ++i) {
        float a0 = (float)i / segments * 2.f * PI;
        float a1 = (float)(i + 1) / segments * 2.f * PI;
        detail::RVertex p0{cx + std::cos(a0) * radius, cy + std::sin(a0) * radius, 0.5f, 0.5f, c.r, c.g, c.b, c.a};
        detail::RVertex p1{cx + std::cos(a1) * radius, cy + std::sin(a1) * radius, 0.5f, 0.5f, c.r, c.g, c.b, c.a};
        detail::pushTri(0, center, p0, p1);
    }
}
void Renderer::drawCircleAA(float cx, float cy, float radius, const Color& c, int segments) {
    if (radius <= 0) return;
    float innerRadius = std::max(0.f, radius - 1.f);
    drawCircle(cx, cy, innerRadius, c, segments);
    segments = std::max(3, segments);
    Color edge = c.withAlpha(0);
    for (int i = 0; i < segments; ++i) {
        float a0 = (float)i / segments * 2 * PI, a1 = (float)(i + 1) / segments * 2 * PI;
        detail::RVertex i0{cx + std::cos(a0) * innerRadius, cy + std::sin(a0) * innerRadius, .5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex i1{cx + std::cos(a1) * innerRadius, cy + std::sin(a1) * innerRadius, .5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex o0{cx + std::cos(a0) * radius, cy + std::sin(a0) * radius, .5f,.5f,edge.r,edge.g,edge.b,edge.a};
        detail::RVertex o1{cx + std::cos(a1) * radius, cy + std::sin(a1) * radius, .5f,.5f,edge.r,edge.g,edge.b,edge.a};
        detail::pushTri(0, i0, i1, o1); detail::pushTri(0, i0, o1, o0);
    }
}
void Renderer::drawRectOutline(float x, float y, float w, float h, float t, const Color& c) {
    if (t <= 0 || w <= 0 || h <= 0) return;
    t = std::min(t, std::min(w, h) * 0.5f);
    drawRect(x, y, w, t, c); drawRect(x, y + h - t, w, t, c);
    if (h > 2 * t) { drawRect(x, y + t, t, h - 2 * t, c); drawRect(x + w - t, y + t, t, h - 2 * t, c); }
}
void Renderer::drawCircleOutline(float cx, float cy, float radius, float t, const Color& c, int segments) {
    if (radius <= 0 || t <= 0) return;
    float inner = std::max(0.f, radius - t);
    segments = std::max(3, segments);
    for (int i = 0; i < segments; ++i) {
        float a0 = (float)i / segments * 2 * PI, a1 = (float)(i + 1) / segments * 2 * PI;
        detail::RVertex o0{cx+std::cos(a0)*radius,cy+std::sin(a0)*radius,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex o1{cx+std::cos(a1)*radius,cy+std::sin(a1)*radius,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex i0{cx+std::cos(a0)*inner,cy+std::sin(a0)*inner,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex i1{cx+std::cos(a1)*inner,cy+std::sin(a1)*inner,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::pushTri(0, i0, o0, o1); detail::pushTri(0, i0, o1, i1);
    }
}
void Renderer::drawRectGradient(float x, float y, float w, float h, Color tl, Color tr, Color bl, Color br, float rotDeg) {
    detail::pushGradientRect(x, y, w, h, tl, tr, bl, br, rotDeg);
}
void Renderer::drawRoundRect(float x, float y, float w, float h, float radius, const Color& c, int cornerSegments) {
    if (w <= 0 || h <= 0) return;
    radius = clampf(radius, 0, std::min(w, h) * .5f);
    if (radius == 0) { drawRect(x, y, w, h, c); return; }
    drawRect(x + radius, y, w - 2 * radius, h, c);
    drawRect(x, y + radius, radius, h - 2 * radius, c);
    drawRect(x + w - radius, y + radius, radius, h - 2 * radius, c);
    const float centers[4][3] = {{x+radius,y+radius,180},{x+w-radius,y+radius,270},
                                 {x+w-radius,y+h-radius,0},{x+radius,y+h-radius,90}};
    cornerSegments = std::max(1, cornerSegments);
    for (const auto& q : centers) for (int i = 0; i < cornerSegments; ++i) {
        float a0 = deg2rad(q[2] + 90.f * i / cornerSegments), a1 = deg2rad(q[2] + 90.f * (i+1) / cornerSegments);
        detail::RVertex m{q[0],q[1],.5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex p0{q[0]+std::cos(a0)*radius,q[1]+std::sin(a0)*radius,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::RVertex p1{q[0]+std::cos(a1)*radius,q[1]+std::sin(a1)*radius,.5f,.5f,c.r,c.g,c.b,c.a};
        detail::pushTri(0,m,p0,p1);
    }
}
void Renderer::drawNineSlice(const Texture& tex, float x, float y, float w, float h,
                             float left, float right, float top, float bottom) {
    if (!tex.valid() || w <= 0 || h <= 0) return;
    float tw = (float)tex.width(), th = (float)tex.height();
    float sx[4] = {0, clampf(left, 0, tw), 0, tw};
    float sy[4] = {0, clampf(top, 0, th), 0, th};
    sx[2] = clampf(tw - right, sx[1], tw);
    sy[2] = clampf(th - bottom, sy[1], th);
    float dl = std::min(sx[1]-sx[0], w*.5f), dr = std::min(sx[3]-sx[2], w*.5f);
    float dt = std::min(sy[1]-sy[0], h*.5f), db = std::min(sy[3]-sy[2], h*.5f);
    float dx[4] = {x, x+dl, x+w-dr, x+w}, dy[4] = {y,y+dt,y+h-db,y+h};
    for (int row=0; row<3; ++row) for (int col=0; col<3; ++col) {
        if (dx[col+1] <= dx[col] || dy[row+1] <= dy[row]) continue;
        drawTexture(tex,dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row],
                    Color::White,0,sx[col]/tw,sy[row]/th,sx[col+1]/tw,sy[row+1]/th);
    }
}
void Renderer::drawTexture(const Texture& tex, float x, float y, float w, float h,
                           const Color& tint, float rotDeg, float u0, float v0, float u1, float v1) {
    if (!tex.valid()) return;
    detail::pushRectVerts(tex.id(), x, y, w, h, tint, rotDeg, u0, v0, u1, v1);
}
void Renderer::drawTextureFlipped(const Texture& tex, float x, float y, float w, float h,
                                  bool flipX, bool flipY, const Color& tint, float rotDeg) {
    drawTexture(tex,x,y,w,h,tint,rotDeg,flipX?1.f:0.f,flipY?1.f:0.f,flipX?0.f:1.f,flipY?0.f:1.f);
}
void Renderer::drawSprite(const Sprite& s) {
    if (!s.tex || !s.tex->valid()) return;
    float w = s.autoSize ? (float)s.tex->width() : s.size.x;   // autoSize 跟随纹理
    float h = s.autoSize ? (float)s.tex->height() : s.size.y;
    w *= s.scale.x; h *= s.scale.y;
    float u0 = s.uv.x, v0 = s.uv.y, u1 = s.uv.x + s.uv.w, v1 = s.uv.y + s.uv.h;
    if (s.flipX) std::swap(u0, u1);
    if (s.flipY) std::swap(v0, v1);
    detail::pushRectAtAnchor(s.tex->id(), s.pos.x, s.pos.y, w, h, s.color, s.rotation, s.anchor.x, s.anchor.y, u0, v0, u1, v1);
}
void Renderer::setBlendAdditive(bool additive) {
    auto& rs = detail::RS();
    if (rs.additive != additive) { flush(); rs.additive = additive; }
}
int Renderer::getDrawCallCount() { return detail::RS().drawCalls; }
void Renderer::resetStats() { detail::RS().drawCalls = 0; }
void Renderer::debugDrawRect(const Rect& r, const Color& c) {
    drawRectOutline(r.x,r.y,r.w,r.h,1,c);
    drawLine(r.x,r.y+r.h*.5f,r.x+r.w,r.y+r.h*.5f,1,c,LineCap::Butt);
    drawLine(r.x+r.w*.5f,r.y,r.x+r.w*.5f,r.y+r.h,1,c,LineCap::Butt);
}
void Renderer::debugDrawCircle(Vec2 p,float r,const Color& c) { drawCircleOutline(p.x,p.y,r,1,c); }
void Renderer::debugDrawLine(Vec2 a,Vec2 b,const Color& c) { drawLine(a.x,a.y,b.x,b.y,1,c); }
void Renderer::debugDrawGrid(float spacing,int cols,int rows,const Color& c) {
    if (spacing <= 0 || cols < 0 || rows < 0) return;
    for(int x=0;x<=cols;++x) drawLine(x*spacing,0,x*spacing,rows*spacing,1,c,LineCap::Butt);
    for(int y=0;y<=rows;++y) drawLine(0,y*spacing,cols*spacing,y*spacing,1,c,LineCap::Butt);
}
void Renderer::flush() {
    auto& rs = detail::RS();
    if (!rs.inited || rs.idx.empty()) return;
    Mat4 proj = rs.cam ? rs.cam->viewProj((float)rs.vw, (float)rs.vh)
                       : Mat4::ortho(0, (float)rs.vw, (float)rs.vh, 0);
    glUseProgram(rs.prog);
    glBlendFunc(rs.additive ? LGNITE_GL_ONE : LGNITE_GL_SRC_ALPHA,
                rs.additive ? LGNITE_GL_ONE : LGNITE_GL_ONE_MINUS_SRC_ALPHA);
    glUniformMatrix4fv(rs.locProj, 1, LGNITE_GL_FALSE, proj.m);
    glActiveTexture(LGNITE_GL_TEXTURE0);
    glBindTexture(LGNITE_GL_TEXTURE_2D, rs.curTex ? rs.curTex : rs.white);
    glUniform1i(rs.locTex, 0);
    glBindVertexArray(rs.vao);
    glBindBuffer(LGNITE_GL_ARRAY_BUFFER, rs.vbo);
    glBufferSubData(LGNITE_GL_ARRAY_BUFFER, 0, (GLsizeiptr)(rs.verts.size() * sizeof(detail::RVertex)), rs.verts.data());
    glBindBuffer(LGNITE_GL_ELEMENT_ARRAY_BUFFER, rs.ibo);
    glBufferSubData(LGNITE_GL_ELEMENT_ARRAY_BUFFER, 0, (GLsizeiptr)(rs.idx.size() * sizeof(uint32_t)), rs.idx.data());
    glDrawElements(LGNITE_GL_TRIANGLES, (GLsizei)rs.idx.size(), LGNITE_GL_UNSIGNED_INT, nullptr);
    ++rs.drawCalls;
    glBindVertexArray(0);
    rs.verts.clear();
    rs.idx.clear();
}

// ============================== Sprite / SpriteSheet / Animation ==============================
void Sprite::draw() const { Renderer::drawSprite(*this); }
Rect Sprite::bounds() const {
    float w = autoSize ? (tex ? (float)tex->width() : 0) : size.x;
    float h = autoSize ? (tex ? (float)tex->height() : 0) : size.y;
    return {pos.x, pos.y, w * scale.x, h * scale.y};
}
Rect Sprite::boundsRotated() const {
    Rect b = bounds();
    if (rotation == 0) return b;
    Vec2 pivot{b.x + b.w * anchor.x, b.y + b.h * anchor.y};
    Vec2 corners[4] = {{b.x,b.y},{b.x+b.w,b.y},{b.x+b.w,b.y+b.h},{b.x,b.y+b.h}};
    float minX = 0, maxX = 0, minY = 0, maxY = 0;
    for (int i=0;i<4;++i) {
        Vec2 q = pivot + (corners[i] - pivot).rotated(rotation);
        if (i==0) minX=maxX=q.x,minY=maxY=q.y;
        else { minX=std::min(minX,q.x); maxX=std::max(maxX,q.x); minY=std::min(minY,q.y); maxY=std::max(maxY,q.y); }
    }
    return {minX,minY,maxX-minX,maxY-minY};
}
bool circleCollide(Vec2 a,float ra,Vec2 b,float rb) { return ra >= 0 && rb >= 0 && Vec2::dist(a,b) <= ra + rb; }

void SpriteSheet::set(const Texture& t, int frameW_, int frameH_) {
    tex = &t; frameW = frameW_; frameH = frameH_;
    cols = frameW > 0 ? t.width() / frameW : 0;
    rows = frameH > 0 ? t.height() / frameH : 0;
}
FrameUV SpriteSheet::frame(int index) const {
    if (!tex || cols <= 0 || rows <= 0) return {0, 0, 1, 1};
    index = index % count();
    int c = index % cols, r = index / cols;
    float tw = (float)tex->width(), th = (float)tex->height();
    return {c * frameW / tw, r * frameH / th, (c + 1) * frameW / tw, (r + 1) * frameH / th};
}

void Animation::setRange(int first, int last) {
    frames.clear();
    for (int i = first; i <= last; ++i) frames.push_back(i);
    reset();
}
void Animation::update(float dt) {
    if (!playing || frames.empty()) return;
    time += dt;
    float step = 1.f / fps;
    while (time >= step) {
        time -= step;
        if (++cursor >= (int)frames.size())
            cursor = loop ? 0 : (int)frames.size() - 1;
    }
}
int Animation::current() const { return frames.empty() ? 0 : frames[cursor]; }
bool Animation::finished() const { return !loop && !frames.empty() && cursor == (int)frames.size() - 1; }

// ============================== Camera2D ==============================
void Camera2D::setZoom(float z) { zoom = clampf(z, zoomMin, zoomMax); }
void Camera2D::follow(Vec2 target, float dt, float screenW, float screenH) {
    zoom = clampf(zoom, zoomMin, zoomMax);
    float t = 1.f - std::exp(-followSmooth * dt);   // 帧率无关的平滑
    pos += (target - pos) * t;
    if (clampEnabled && screenW > 0 && screenH > 0) {
        float hw = screenW * 0.5f / zoom, hh = screenH * 0.5f / zoom;
        if (clampRect.w >= hw * 2) pos.x = clampf(pos.x, clampRect.x + hw, clampRect.x + clampRect.w - hw);
        if (clampRect.h >= hh * 2) pos.y = clampf(pos.y, clampRect.y + hh, clampRect.y + clampRect.h - hh);
    }
    if (shakeTime_ > 0) {
        float amount = shakeDuration_ > 0 ? shakeIntensity_ * shakeTime_ / shakeDuration_ : 0;
        shakeOffset_ = {randf(-amount, amount), randf(-amount, amount)};
        shakeTime_ = std::max(0.f, shakeTime_ - std::max(0.f, dt));
        if (shakeTime_ == 0) shakeOffset_ = {0, 0};
    } else shakeOffset_ = {0, 0};
}
Mat4 Camera2D::viewProj(float w, float h) const {
    float z = clampf(zoom, zoomMin, zoomMax);
    Vec2 p = pos + shakeOffset_;
    Mat4 v = Mat4::translation(w * 0.5f, h * 0.5f)
           * Mat4::scaling(z, z)
           * Mat4::rotationZ(-rotation)
           * Mat4::translation(-p.x, -p.y);
    return Mat4::ortho(0, w, h, 0) * v;
}
Vec2 Camera2D::screenToWorld(Vec2 s, float w, float h) const {
    float z = clampf(zoom, zoomMin, zoomMax);
    float dx = (s.x - w * 0.5f) / z, dy = (s.y - h * 0.5f) / z;
    float r = deg2rad(rotation), c = std::cos(r), sn = std::sin(r);
    Vec2 p = pos + shakeOffset_;
    return {p.x + dx * c - dy * sn, p.y + dx * sn + dy * c};
}
Vec2 Camera2D::worldToScreen(Vec2 p, float w, float h) const {
    float z = clampf(zoom, zoomMin, zoomMax);
    Vec2 camera = pos + shakeOffset_;
    float dx = p.x - camera.x, dy = p.y - camera.y;
    float r = deg2rad(-rotation), c = std::cos(r), sn = std::sin(r);
    return {w * 0.5f + (dx * c - dy * sn) * z, h * 0.5f + (dx * sn + dy * c) * z};
}
void Camera2D::shake(float intensity, float duration) {
    if (duration <= 0 || intensity <= 0) { shakeTime_ = shakeDuration_ = shakeIntensity_ = 0; shakeOffset_ = {0,0}; return; }
    shakeDuration_ = shakeTime_ = duration; shakeIntensity_ = intensity;
}

// ============================== Font ==============================
// 基于 stbtt_PackFontRanges 的可指定字符集烘焙; drawText 走 UTF-8 解码查表
namespace {
// UTF-8 解码: 取下一个码点并推进指针(非法序列按 U+FFFD 处理)
uint32_t utf8Next(const char*& p) {
    const unsigned char* s = (const unsigned char*)p;
    uint32_t cp; int n;
    if (s[0] < 0x80) { cp = s[0]; n = 1; }
    else if ((s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; n = 2; }
    else if ((s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; n = 3; }
    else if ((s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; n = 4; }
    else { cp = 0xFFFD; n = 1; }
    for (int i = 1; i < n; ++i) {
        if ((s[i] & 0xC0) != 0x80) { cp = 0xFFFD; n = 1; break; }
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    p += n;
    return cp;
}
}

Font::~Font() {
    if (pdata_) { delete[] static_cast<stbtt_packedchar*>(pdata_); pdata_ = nullptr; }
}
Font::Font(Font&& o) noexcept
    : atlas_(std::move(o.atlas_)), pdata_(o.pdata_), cps_(std::move(o.cps_)),
      idx_(std::move(o.idx_)), atlasW_(o.atlasW_), atlasH_(o.atlasH_), size_(o.size_), ok_(o.ok_),
      oversampleX_(o.oversampleX_), oversampleY_(o.oversampleY_),
      lineSpacing(o.lineSpacing), letterSpacing(o.letterSpacing) {
    o.pdata_ = nullptr; o.ok_ = false;
}
Font& Font::operator=(Font&& o) noexcept {
    if (this != &o) {
        if (pdata_) { delete[] static_cast<stbtt_packedchar*>(pdata_); pdata_ = nullptr; }   // 手动释放, 不调析构
        atlas_ = std::move(o.atlas_);
        cps_ = std::move(o.cps_); idx_ = std::move(o.idx_);
        pdata_ = o.pdata_; atlasW_ = o.atlasW_; atlasH_ = o.atlasH_; size_ = o.size_; ok_ = o.ok_;
        oversampleX_ = o.oversampleX_; oversampleY_ = o.oversampleY_;
        lineSpacing = o.lineSpacing; letterSpacing = o.letterSpacing;
        o.pdata_ = nullptr; o.ok_ = false;
    }
    return *this;
}
bool Font::load(const char* path, float pixelSize) {
    std::vector<unsigned char> bytes;
    if (!detail::readFileBytes(path, bytes)) { Log::error("[Font] 读取失败: %s", path); return false; }
    return loadFromMemory(bytes.data(), (int)bytes.size(), pixelSize);
}
bool Font::load(const char* path, float pixelSize, const uint32_t* codepoints, int count) {
    std::vector<unsigned char> bytes;
    if (!detail::readFileBytes(path, bytes)) { Log::error("[Font] 读取失败: %s", path); return false; }
    return loadFromMemory(bytes.data(), (int)bytes.size(), pixelSize, codepoints, count);
}
bool Font::loadFromString(const char* path, float pixelSize, const char* sampleText) {
    if (!sampleText) { Log::error("[Font] loadFromString 收到空文本"); return false; }
    std::vector<uint32_t> cps;
    std::unordered_set<uint32_t> unique;
    const char* p = sampleText;
    while (*p) {
        uint32_t cp = utf8Next(p);
        if (cp != '\n' && cp != '\r' && unique.insert(cp).second) cps.push_back(cp);
    }
    if (cps.empty()) { Log::error("[Font] loadFromString 没有可烘焙字符"); return false; }
    return load(path, pixelSize, cps.data(), (int)cps.size());
}
bool Font::loadCJK(const char* path, float pixelSize) {
    struct OversampleGuard {
        int& x;
        int& y;
        int oldX, oldY;
        ~OversampleGuard() { x = oldX; y = oldY; }
    } guard{oversampleX_, oversampleY_, oversampleX_, oversampleY_};
    oversampleX_ = oversampleY_ = 1;
    std::vector<uint32_t> cps;
    cps.reserve(0x9FFF - 0x4E00 + 1);
    for (uint32_t cp = 0x4E00; cp <= 0x9FFF; ++cp) cps.push_back(cp);
    return load(path, pixelSize, cps.data(), (int)cps.size());
}
void Font::setOversample(int x, int y) {
    oversampleX_ = std::max(1, std::min(x, 8));
    oversampleY_ = std::max(1, std::min(y, 8));
}
bool Font::loadFromMemory(const unsigned char* data, int len, float pixelSize,
                          const uint32_t* codepoints, int count) {
    if (!data || len <= 0 || !std::isfinite(pixelSize) || pixelSize <= 0) {
        Log::error("[Font] 无效字体数据或字号"); return false;
    }
    // 默认字符集: ASCII 32~126
    std::vector<uint32_t> cps;
    if (codepoints && count > 0) cps.assign(codepoints, codepoints + count);
    else for (uint32_t c = 32; c < 127; ++c) cps.push_back(c);
    count = (int)cps.size();
    if (count <= 0) { Log::error("[Font] 字符集为空"); return false; }

    auto* pdata = new stbtt_packedchar[count];
    std::vector<int> cpsInt;   // 显式转换避免 MSVC /W4 C4244 窄化告警
    cpsInt.reserve(cps.size());
    for (uint32_t c : cps) cpsInt.push_back(static_cast<int>(c));
    // 按字符数量/字号估算图集边长(2 的幂), 烘焙失败则放大重试
    int dim = 64;
    double need = std::sqrt((double)count) * pixelSize * 1.35 *
                  std::sqrt((double)oversampleX_ * oversampleY_);
    while (dim < need && dim < 4096) dim <<= 1;
    if (dim > 4096) dim = 4096;
    std::vector<unsigned char> bitmap;
    bool packed = false;
    int goodDim = 0;   // 显式记录成功那轮的图集边长(不依赖循环变量事后回退)
    for (; dim <= 4096 && !packed; dim <<= 1) {
        bitmap.assign((size_t)dim * dim, 0);
        stbtt_pack_context ctx;
        if (!stbtt_PackBegin(&ctx, bitmap.data(), dim, dim, 0, 1, nullptr)) continue;
        stbtt_PackSetOversampling(&ctx, (unsigned char)oversampleX_, (unsigned char)oversampleY_);
        stbtt_pack_range range{};
        range.font_size = pixelSize;
        range.first_unicode_codepoint_in_range = 0;
        range.array_of_unicode_codepoints = cpsInt.data();
        range.num_chars = count;
        range.chardata_for_range = pdata;
        if (stbtt_PackFontRanges(&ctx, data, 0, &range, 1) != 0) {
            packed = true;
            goodDim = dim;
        }
        stbtt_PackEnd(&ctx);
    }
    if (!packed) {   // stbtt 打包返回 0 才是失败
        Log::error("[Font] 烘焙失败: %d 字符 @ %.0fpx(图集 4096 仍放不下)", count, pixelSize);
        delete[] pdata;
        return false;
    }
    dim = goodDim;
    Log::info("[Font] 烘焙完成: %d 字符, 图集 %dx%d", count, dim, dim);
    // 单通道 alpha 扩展为 RGBA, 与渲染管线统一
    std::vector<unsigned char> rgba((size_t)dim * dim * 4);
    for (size_t i = 0; i < (size_t)dim * dim; ++i) {
        rgba[i * 4 + 0] = 255; rgba[i * 4 + 1] = 255; rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = bitmap[i];
    }
    if (pdata_) { delete[] static_cast<stbtt_packedchar*>(pdata_); pdata_ = nullptr; }
    atlas_.destroy();
    pdata_ = pdata;
    cps_ = std::move(cps);
    idx_.clear();
    for (int i = 0; i < count; ++i) idx_[cps_[i]] = i;
    atlasW_ = atlasH_ = dim;
    size_ = pixelSize;
    ok_ = atlas_.create(dim, dim, rgba.data());
    return ok_;
}
void Font::drawText(float x, float y, const char* utf8, const Color& c, float scale) const {
    if (!ok_ || !utf8) return;
    auto* pdata = static_cast<stbtt_packedchar*>(pdata_);
    float px = 0, py = 0;              // 笔位从 0 起算, 绘制时叠加 (x,y) 并缩放
    const char* p = utf8;
    while (*p) {
        uint32_t cp = utf8Next(p);
        if (cp == '\n') { px = 0; py += size_ * lineSpacing; continue; }
        auto it = idx_.find(cp);
        if (it == idx_.end()) continue;          // 未烘焙字符跳过
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(pdata, atlasW_, atlasH_, it->second, &px, &py, &q, 1);
        Renderer::drawTexture(atlas_, x + q.x0 * scale, y + q.y0 * scale,
                              (q.x1 - q.x0) * scale, (q.y1 - q.y0) * scale,
                              c, 0, q.s0, q.t0, q.s1, q.t1);
        px += letterSpacing;
    }
}
float Font::textWidth(const char* utf8, float scale) const {
    if (!ok_ || !utf8) return 0;
    auto* pdata = static_cast<stbtt_packedchar*>(pdata_);
    float px = 0, py = 0, maxX = 0;
    bool lineHasGlyph = false;
    const char* p = utf8;
    while (*p) {
        uint32_t cp = utf8Next(p);
        if (cp == '\n') {
            float width = px - (lineHasGlyph ? letterSpacing : 0);
            if (width > maxX) maxX = width;
            px = 0; py += size_ * lineSpacing; lineHasGlyph = false; continue;
        }
        auto it = idx_.find(cp);
        if (it == idx_.end()) continue;
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(pdata, atlasW_, atlasH_, it->second, &px, &py, &q, 1);
        px += letterSpacing;
        lineHasGlyph = true;
    }
    float width = px - (lineHasGlyph ? letterSpacing : 0);
    if (width > maxX) maxX = width;
    return maxX * scale;
}
float Font::textHeight(const char* utf8, float scale) const {
    if (!ok_ || !utf8) return 0;
    int lines = 1;
    for (const char* p = utf8; *p; ++p) if (*p == '\n') ++lines;
    return (float)lines * size_ * lineSpacing * scale;
}
void Font::drawTextOutline(float x,float y,const char* text,const Color& fill,const Color& outline,
                           float thickness,float scale) const {
    float d = std::max(0.f, thickness);
    if (d > 0) {
        drawText(x-d,y-d,text,outline,scale); drawText(x,y-d,text,outline,scale);
        drawText(x+d,y-d,text,outline,scale); drawText(x-d,y,text,outline,scale);
        drawText(x+d,y,text,outline,scale); drawText(x-d,y+d,text,outline,scale);
        drawText(x,y+d,text,outline,scale); drawText(x+d,y+d,text,outline,scale);
    }
    drawText(x,y,text,fill,scale);
}
void Font::drawTextAligned(float x,float y,const char* text,const Color& c,Align align,float scale) const {
    if (align == Align::Center) x -= textWidth(text,scale)*.5f;
    else if (align == Align::Right) x -= textWidth(text,scale);
    drawText(x,y,text,c,scale);
}
void Font::drawTextShadow(float x,float y,const char* text,const Color& fill,Color shadow,
                          float offsetX,float offsetY,float scale) const {
    drawText(x+offsetX,y+offsetY,text,shadow,scale);
    drawText(x,y,text,fill,scale);
}
void Font::drawTextf(float x,float y,const Color& c,float scale,const char* fmt,...) const {
    if (!fmt) return;
    va_list args;
    va_start(args,fmt);
    drawTextV(x,y,c,scale,fmt,args);
    va_end(args);
}
void Font::drawTextV(float x,float y,const Color& c,float scale,const char* fmt,va_list args) const {
    char buffer[2048];
    int n=std::vsnprintf(buffer,sizeof(buffer),fmt,args);
    if (n < 0) { Log::error("[Font] drawTextf 格式化失败"); return; }
    if ((size_t)n >= sizeof(buffer)) Log::warn("[Font] drawTextf 文本超过缓冲区, 已截断");
    drawText(x,y,buffer,c,scale);
}

// 渲染器便捷转发
void Renderer::drawText(const Font& font, float x, float y, const char* text, const Color& c, float scale) {
    font.drawText(x, y, text, c, scale);
}
void Renderer::drawTextf(const Font& font,float x,float y,const Color& c,float scale,const char* fmt,...) {
    if (!fmt) return;
    va_list args;
    va_start(args,fmt);
    font.drawTextV(x,y,c,scale,fmt,args);
    va_end(args);
}

// ============================== ParticleSystem ==============================
namespace detail {
float applyEase(ParticleSystem::Ease ease, float t) {
    t = clampf(t, 0, 1);
    switch (ease) {
        case ParticleSystem::Ease::EaseIn: return t * t;
        case ParticleSystem::Ease::EaseOut: return 1 - (1 - t) * (1 - t);
        case ParticleSystem::Ease::EaseInOut: return t < .5f ? 2*t*t : 1-2*(1-t)*(1-t);
        default: return t;
    }
}
}
void ParticleSystem::emit(int n) {
    for (int i = 0; i < n && (int)parts_.size() < maxParticles; ++i) {
        Particle p;
        p.pos = pos;
        if (shape == EmitShape::Circle) {
            float a = randf(0, 2 * PI), r = shapeRadius * std::sqrt(randf(0, 1));
            p.pos += Vec2(std::cos(a) * r, std::sin(a) * r);
        } else if (shape == EmitShape::Rect) {
            p.pos += Vec2(randf(-shapeSize.x*.5f,shapeSize.x*.5f),randf(-shapeSize.y*.5f,shapeSize.y*.5f));
        } else if (shape == EmitShape::Line) {
            p.pos += shapeLineEnd * randf(0, 1);
        }
        p.vel = {randf(velMin.x, velMax.x), randf(velMin.y, velMax.y)};
        p.maxLife = p.life = randf(lifeMin, lifeMax);
        p.size0 = size0; p.size1 = size1;
        p.c0 = color0; p.c1 = color1;
        p.rotation = randf(rotationMin, rotationMax);
        p.rotationVel = randf(rotationVelMin, rotationVelMax);
        parts_.push_back(p);
    }
}
void ParticleSystem::update(float dt) {
    if (emitting) {
        acc_ += rate * dt;
        int n = (int)acc_;
        acc_ -= (float)n;
        if (n > 0) emit(n);
    } else {
        acc_ = 0;   // 停止发射时清零, 防止重新开启后瞬间爆发
    }
    for (size_t i = 0; i < parts_.size();) {
        Particle& p = parts_[i];
        p.life -= dt;
        if (p.life <= 0) { parts_[i] = parts_.back(); parts_.pop_back(); continue; }
        p.vel += gravity * dt;
        p.pos += p.vel * dt;
        p.rotation += p.rotationVel * dt;
        ++i;
    }
}
void ParticleSystem::draw() const {
    if (blend == Blend::Additive) Renderer::setBlendAdditive(true);
    for (const Particle& p : parts_) {
        float t = 1.f - p.life / p.maxLife;   // 0(新生) -> 1(死亡)
        float s = lerpf(p.size0, p.size1, detail::applyEase(sizeEase, t));
        if (s <= 0) continue;
        Color c = Color::lerp(p.c0, p.c1, detail::applyEase(colorEase, t));
        if (tex && tex->valid())   // 纹理粒子: 带尺寸插值与颜色 tint
            Renderer::drawTexture(*tex, p.pos.x - s * 0.5f, p.pos.y - s * 0.5f, s, s, c, p.rotation);
        else                       // 方块粒子(默认)
            Renderer::drawRect(p.pos.x - s * 0.5f, p.pos.y - s * 0.5f, s, s, c, p.rotation);
    }
    if (blend == Blend::Additive) Renderer::setBlendAdditive(false);
}

// ============================== Shader ==============================
Shader::~Shader() { destroy(); }
Shader::Shader(Shader&& o) noexcept : prog_(o.prog_) { o.prog_ = 0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) { destroy(); prog_ = o.prog_; o.prog_ = 0; }
    return *this;
}
bool Shader::compile(const char* vsSrc, const char* fsSrc) {
    destroy();
    GLuint vs = detail::compileShader(LGNITE_GL_VERTEX_SHADER, vsSrc);
    GLuint fs = detail::compileShader(LGNITE_GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return false;
    prog_ = detail::linkProgram(vs, fs);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog_ != 0;
}
void Shader::destroy() {
    if (prog_) {
        if (glfwGetCurrentContext()) glDeleteProgram(prog_);
        prog_ = 0;
    }
}
// 注意: 上下文销毁后(如 glfwTerminate 之后)这些调用均为空操作, 防崩
void Shader::use() const { if (prog_ && glfwGetCurrentContext()) glUseProgram(prog_); }
void Shader::setInt(const char* n, int v) const { if (prog_ && glfwGetCurrentContext()) glUniform1i(glGetUniformLocation(prog_, n), v); }
void Shader::setFloat(const char* n, float v) const { if (prog_ && glfwGetCurrentContext()) glUniform1f(glGetUniformLocation(prog_, n), v); }
void Shader::setVec4(const char* n, float x, float y, float z, float w) const {
    if (prog_ && glfwGetCurrentContext()) glUniform4f(glGetUniformLocation(prog_, n), x, y, z, w);
}
void Shader::setMat4(const char* n, const Mat4& m) const {
    if (prog_ && glfwGetCurrentContext()) glUniformMatrix4fv(glGetUniformLocation(prog_, n), 1, LGNITE_GL_FALSE, m.m);
}

// ============================== Tilemap ==============================
void Tilemap::create(int w, int h, int tileSize) {
    // 注意: create 会清空渲染相关状态, 必须在此之后重新 setTileset
    w_ = w; h_ = h; tile_ = tileSize;
    data_.assign((size_t)w * h, -1);
    uv_.clear(); cols_ = 1; rows_ = 1; cellW_ = cellH_ = 0;
    tileset_ = nullptr; warned_ = false;
    layers_.clear(); solidIds_.clear(); autotile_ = false;
}
void Tilemap::setTileset(const Texture& tex, int cols) {
    setTileset(tex, cols, 0);   // rows=0: 按正方形网格从 cellW_ 反推行数
}
void Tilemap::setTileset(const Texture& tex, int cols, int rows) {
    uv_.clear();   // 先清旧 UV, 任何提前 return 都不残留
    tileset_ = &tex;
    cols_ = cols > 0 ? cols : 1;
    if (tex.width() <= 0 || tex.height() <= 0) return;
    cellW_ = (float)tex.width() / cols_;
    if (rows > 0) {
        rows_ = rows;
        cellH_ = (float)tex.height() / rows_;
    } else {
        cellH_ = cellW_;                            // 正方形网格
        rows_ = cellW_ > 0 ? (int)(tex.height() / cellW_) : 0;
        if (cellW_ > 0 && tex.height() % (int)cellW_ != 0)
            Log::warn("[Tilemap] 图集高度 %d 不是块高 %.0f 的整数倍, 末行可能被裁掉", tex.height(), cellW_);
        if (rows_ < 1) rows_ = 1;
    }
    // 预计算全部瓦片 UV, draw 时直接查表
    uv_.clear();
    uv_.reserve((size_t)cols_ * rows_);
    float tw = (float)tex.width(), th = (float)tex.height();
    for (int id = 0; id < cols_ * rows_; ++id) {
        int c = id % cols_, r = id / cols_;
        uv_.push_back({c * cellW_ / tw, r * cellH_ / th, (c + 1) * cellW_ / tw, (r + 1) * cellH_ / th});
    }
}
void Tilemap::set(int x, int y, int id) {
    if (x >= 0 && x < w_ && y >= 0 && y < h_) data_[(size_t)y * w_ + x] = id;
}
int Tilemap::get(int x, int y) const {
    return (x >= 0 && x < w_ && y >= 0 && y < h_) ? data_[(size_t)y * w_ + x] : -1;
}
void Tilemap::addLayer(const char* name) {
    Layer layer; layer.data.assign((size_t)w_ * h_, -1); layer.name = name ? name : "";
    layers_.push_back(std::move(layer));
}
void Tilemap::set(int layer,int x,int y,int id) {
    if (layer == 0) { set(x,y,id); return; }
    if (layer < 0 || (size_t)layer > layers_.size() || x < 0 || x >= w_ || y < 0 || y >= h_) return;
    layers_[layer-1].data[(size_t)y*w_+x] = id;
}
int Tilemap::get(int layer,int x,int y) const {
    if (layer == 0) return get(x,y);
    if (layer < 0 || (size_t)layer > layers_.size() || x < 0 || x >= w_ || y < 0 || y >= h_) return -1;
    return layers_[layer-1].data[(size_t)y*w_+x];
}
void Tilemap::setLayerParallax(const char* name,float p) {
    const std::string key = name ? name : "";
    for (auto& layer : layers_) if (layer.name == key) { layer.parallax = std::max(0.f,p); return; }
    Log::warn("[Tilemap] 未找到图层: %s", key.c_str());
}
bool Tilemap::hasLayer(const char* name) const {
    const std::string key = name ? name : "";
    return std::any_of(layers_.begin(), layers_.end(),
                       [&key](const Layer& layer) { return layer.name == key; });
}
void Tilemap::setLayerVisible(const char* name,bool visible) {
    const std::string key=name?name:"";
    for(auto& layer:layers_) if(layer.name==key) { layer.visible=visible; return; }
    Log::warn("[Tilemap] 未找到图层: %s",key.c_str());
}
void Tilemap::setTileSolid(int id,bool solid) {
    if (solid) solidIds_.insert(id); else solidIds_.erase(id);
}
bool Tilemap::isTileSolid(int x,int y) const { return solidIds_.count(get(x,y)) != 0; }
bool Tilemap::isSolidAt(Vec2 p) const {
    if (tile_ <= 0) return false;
    int x=(int)std::floor(p.x/tile_), y=(int)std::floor(p.y/tile_);
    return isTileSolid(x,y);
}
int Tilemap::autotileId(int x,int y) const {
    int base = get(x,y); if (base < 0) return base;
    static const int dx[8]={0,1,0,-1,-1,1,1,-1}, dy[8]={-1,0,1,0,-1,-1,1,1};
    int count = autotileMode_ == AutotileMode::Bit4 ? 4 : 8, mask = 0;
    for (int i=0;i<count;++i) if (get(x+dx[i],y+dy[i]) == base) mask |= 1<<i;
    int variants = autotileMode_ == AutotileMode::Bit4 ? 16 : 256;
    return base * variants + mask;
}
bool Tilemap::loadCSV(const char* path) {
    std::string text;
    if (!SaveSystem::readText(path, text)) { Log::error("[Tilemap] 读取失败: %s", path); return false; }
    std::vector<int> ids;
    int cols = 0, curCols = 0;
    std::stringstream ss(text);
    std::string cell, line;
    std::vector<std::string> lines;
    while (std::getline(ss, line)) if (!line.empty()) lines.push_back(line);
    for (auto& ln : lines) {
        std::stringstream ls(ln);
        curCols = 0;
        while (std::getline(ls, cell, ',')) { ids.push_back(std::atoi(cell.c_str())); ++curCols; }
        if (cols == 0) cols = curCols;
    }
    if (cols == 0) return false;
    int rows = (int)ids.size() / cols;
    create(cols, rows, tile_ > 0 ? tile_ : 32);
    data_ = std::move(ids);
    return true;
}
Rect Tilemap::tileBounds(int x, int y) const {
    return {(float)x * tile_, (float)y * tile_, (float)tile_, (float)tile_};
}
void Tilemap::draw() const {
    if (!tileset_ || !tileset_->valid() || uv_.empty()) return;
    auto drawLayer = [&](const std::vector<int>& layerData, float parallax) {
      Vec2 shift{0,0};
      const Camera2D* cam = detail::RS().cam;
      if (cam && parallax != 1.f) shift = cam->pos * (1.f - parallax);
      for (int y = 0; y < h_; ++y)
        for (int x = 0; x < w_; ++x) {
            int id = layerData[(size_t)y * w_ + x];
            if (id < 0) continue;
            if (autotile_) {
                static const int dx[8]={0,1,0,-1,-1,1,1,-1}, dy[8]={-1,0,1,0,-1,-1,1,1};
                int base = id, bits = autotileMode_ == AutotileMode::Bit4 ? 4 : 8, mask = 0;
                for (int i=0;i<bits;++i) {
                    int nx=x+dx[i], ny=y+dy[i];
                    int neighbor = nx>=0 && nx<w_ && ny>=0 && ny<h_ ? layerData[(size_t)ny*w_+nx] : -1;
                    if (neighbor == base) mask |= 1<<i;
                }
                id = base * (autotileMode_ == AutotileMode::Bit4 ? 16 : 256) + mask;
            }
            if ((size_t)id >= uv_.size()) {
                if (!warned_) {   // 每实例只警告一次, 避免刷屏(不跨实例共享)
                    Log::warn("[Tilemap] 瓦片 id %d 超出图集范围(%zu 块), 已跳过", id, uv_.size());
                    warned_ = true;
                }
                continue;
            }
            const FrameUV& uv = uv_[id];         // 查预计算表, 不再每块实时算 UV
            Renderer::drawTexture(*tileset_, (float)x * tile_ + shift.x, (float)y * tile_ + shift.y,
                                  (float)tile_, (float)tile_, Color::White, 0, uv.u0, uv.v0, uv.u1, uv.v1);
        }
    };
    drawLayer(data_,1.f);
    for (const auto& layer : layers_) if (layer.visible) drawLayer(layer.data,layer.parallax);
}

// ============================== Actor / ActorGroup ==============================
void ActorGroup::update(float dt) { for (auto& a : items_) if (a->alive) a->update(dt); }
void ActorGroup::draw() { for (auto& a : items_) if (a->alive) a->draw(); }
void ActorGroup::removeDead() {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [](const std::unique_ptr<Actor>& a) { return !a->alive; }),
                 items_.end());
}

// ============================== 音频(miniaudio 封装) ==============================
namespace {
ma_engine* audioEngineRaw() {
    return static_cast<ma_engine*>(AudioEngine::instance().handle());
}
bool audioReady() {
    if (!AudioEngine::instance().ok()) Log::warn("[Audio] AudioEngine 未初始化(App 已自动处理, 手动流程请调用 init)");
    return AudioEngine::instance().ok();
}
}

AudioEngine& AudioEngine::instance() { static AudioEngine e; return e; }
bool AudioEngine::init() {
    if (ok_) return true;
    auto* eng = new ma_engine;
    if (ma_engine_init(nullptr, eng) != MA_SUCCESS) {
        Log::error("[Audio] 音频设备初始化失败");
        delete eng;
        return false;
    }
    engine_ = eng; ok_ = true;
    return true;
}
void AudioEngine::shutdown() {
    if (engine_) { ma_engine_uninit(static_cast<ma_engine*>(engine_)); delete static_cast<ma_engine*>(engine_); engine_ = nullptr; }
    ok_ = false;
}
void AudioEngine::setMasterVolume(float v) { if (ok_) ma_engine_set_volume(audioEngineRaw(), v); }
bool AudioEngine::play(const char* path) {
    if (!audioReady()) return false;
    // 注意: 一次性播放按主音量发声; 需要单独调音量/控制请用 AudioClip
    return ma_engine_play_sound(audioEngineRaw(), path, nullptr) == MA_SUCCESS;
}
namespace { std::vector<AudioClip*>& fadingAudioClips() { static std::vector<AudioClip*> clips; return clips; } }
void AudioEngine::updateFades(float dt) {
    auto& clips = fadingAudioClips();
    for (size_t i = 0; i < clips.size();) {
        AudioClip* clip = clips[i];
        if (!clip || !clip->valid()) { clips.erase(clips.begin()+i); continue; }
        clip->updateFade(std::max(0.f,dt));
        if (clip->fadeStart_ >= clip->fadeDuration_) clips.erase(clips.begin()+i);
        else ++i;
    }
}

AudioBus::~AudioBus() {
    if (group_) {
        if (AudioEngine::instance().ok()) {
            ma_sound_group_uninit(static_cast<ma_sound_group*>(group_));
        } else {
            // 引擎已关时 ma_sound_group_uninit 会访问已销毁引擎(UAF);
            // 仅释放外壳, 内部资源罕见泄漏, 提示用户先销毁 AudioBus 再 shutdown 引擎
            Log::warn("[Audio] AudioBus 在引擎关闭后析构, 内部资源可能泄漏; 请先销毁 AudioBus 再 shutdown 引擎");
        }
        delete static_cast<ma_sound_group*>(group_);
        group_ = nullptr;
    }
}
bool AudioBus::create() {
    if (!audioReady()) return false;
    auto* g = new ma_sound_group;
    if (ma_sound_group_init(audioEngineRaw(), 0, nullptr, g) != MA_SUCCESS) { delete g; return false; }
    group_ = g;
    return true;
}
void AudioBus::setVolume(float v) { if (group_) ma_sound_group_set_volume(static_cast<ma_sound_group*>(group_), v); }

AudioClip::~AudioClip() {
    auto& fades = fadingAudioClips();
    fades.erase(std::remove(fades.begin(), fades.end(), this), fades.end());
    // 注意: 引擎已关闭时(如全局对象在 App::run 返回后析构)仅释放外壳, 避免访问已销毁的引擎
    if (sound_) {
        if (AudioEngine::instance().ok()) ma_sound_uninit(static_cast<ma_sound*>(sound_));
        delete static_cast<ma_sound*>(sound_);
        sound_ = nullptr;
    }
}
AudioClip::AudioClip(AudioClip&& o) noexcept
    : sound_(o.sound_), ok_(o.ok_), fadeStart_(o.fadeStart_), fadeDuration_(o.fadeDuration_),
      fadeFrom_(o.fadeFrom_), fadeTo_(o.fadeTo_), volume_(o.volume_) {
    auto& fades=fadingAudioClips(); for(auto& p:fades) if(p==&o) p=this;
    o.sound_ = nullptr; o.ok_ = false;
}
AudioClip& AudioClip::operator=(AudioClip&& o) noexcept {
    if (this != &o) {
        // 手动释放资源, 禁止显式调用析构(析构后写成员是 UB)
        // 注意: 引擎已关闭时(如全局对象在 App::run 返回后移动赋值)仅释放外壳, 避免访问已销毁的引擎
        auto& fades=fadingAudioClips();
        bool sourceFading=std::find(fades.begin(),fades.end(),&o)!=fades.end();
        fades.erase(std::remove(fades.begin(),fades.end(),this),fades.end());
        fades.erase(std::remove(fades.begin(),fades.end(),&o),fades.end());
        if (sound_) {
            if (AudioEngine::instance().ok()) ma_sound_uninit(static_cast<ma_sound*>(sound_));
            delete static_cast<ma_sound*>(sound_);
        }
        sound_ = o.sound_; ok_ = o.ok_;
        fadeStart_=o.fadeStart_; fadeDuration_=o.fadeDuration_; fadeFrom_=o.fadeFrom_; fadeTo_=o.fadeTo_; volume_=o.volume_;
        if (sourceFading) fades.push_back(this);
        o.sound_ = nullptr; o.ok_ = false;
    }
    return *this;
}
bool AudioClip::load(const char* path, AudioBus* bus) {
    if (!audioReady()) return false;
    auto* s = new ma_sound;
    if (ma_sound_init_from_file(audioEngineRaw(), path, 0,
                                bus ? static_cast<ma_sound_group*>(bus->handle()) : nullptr,
                                nullptr, s) != MA_SUCCESS) {
        Log::error("[Audio] 加载失败: %s", path);
        delete s;
        return false;
    }
    if (sound_) {   // 重载前释放旧资源(此时引擎必然存活, 无需存活检查)
        ma_sound_uninit(static_cast<ma_sound*>(sound_));
        delete static_cast<ma_sound*>(sound_);
    }
    auto& fades=fadingAudioClips();
    fades.erase(std::remove(fades.begin(),fades.end(),this),fades.end());
    fadeStart_=fadeDuration_=0;
    sound_ = s; ok_ = true;
    return true;
}
void AudioClip::play(float volume, bool restart) {
    if (!ok_) return;
    auto* s = static_cast<ma_sound*>(sound_);
    ma_sound_set_volume(s, volume);
    volume_ = volume;
    if (restart) ma_sound_seek_to_pcm_frame(s, 0);   // restart=false: 保持位置继续播
    ma_sound_start(s);
}
void AudioClip::stop() {   // 停止并回到开头(与 MusicPlayer::stop 语义一致)
    if (!ok_) return;
    ma_sound_stop(static_cast<ma_sound*>(sound_));
    ma_sound_seek_to_pcm_frame(static_cast<ma_sound*>(sound_), 0);
}
void AudioClip::pause() { if (ok_) ma_sound_stop(static_cast<ma_sound*>(sound_)); }  // 仅暂停保持位置; 继续播用 play(1.0f, false)
bool AudioClip::isPlaying() const { return ok_ && ma_sound_is_playing(static_cast<ma_sound*>(sound_)); }
void AudioClip::setVolume(float v) { volume_ = v; if (ok_) ma_sound_set_volume(static_cast<ma_sound*>(sound_), v); }
void AudioClip::setLoop(bool loop) { if (ok_) ma_sound_set_looping(static_cast<ma_sound*>(sound_), loop); }
void AudioClip::fadeIn(float duration,float targetVolume) {
    if (!ok_) return;
    targetVolume=clampf(targetVolume,0,1);
    fadeStart_=0; fadeDuration_=std::max(0.f,duration); fadeFrom_=0; fadeTo_=targetVolume;
    if (fadeDuration_ <= 0) { play(targetVolume,false); return; }
    if (!isPlaying()) play(0,true); else setVolume(0);
    auto& fades=fadingAudioClips(); if(std::find(fades.begin(),fades.end(),this)==fades.end()) fades.push_back(this);
}
void AudioClip::fadeOut(float duration) {
    if (!ok_) return;
    fadeStart_=0; fadeDuration_=std::max(0.f,duration); fadeFrom_=clampf(volume_,0,1); fadeTo_=0;
    if (fadeDuration_ <= 0) { setVolume(0); stop(); return; }
    auto& fades=fadingAudioClips(); if(std::find(fades.begin(),fades.end(),this)==fades.end()) fades.push_back(this);
}
void AudioClip::updateFade(float dt) {
    if (!ok_) return;
    fadeStart_=std::min(fadeDuration_,fadeStart_+dt);
    float t=fadeDuration_ > 0 ? fadeStart_/fadeDuration_ : 1;
    setVolume(lerpf(fadeFrom_,fadeTo_,t));
    if(fadeStart_>=fadeDuration_ && fadeTo_==0) {
        ma_sound_stop(static_cast<ma_sound*>(sound_)); // 淡出后保留播放位置；需要回到开头时显式调用 stop()
    }
}
float AudioClip::getPosition() const {
    float seconds=0;
    if(ok_) ma_sound_get_cursor_in_seconds(static_cast<ma_sound*>(sound_),&seconds);
    return seconds;
}
float AudioClip::getDuration() const {
    float seconds=0;
    if(ok_) ma_sound_get_length_in_seconds(static_cast<ma_sound*>(sound_),&seconds);
    return seconds;
}
void AudioClip::seek(float seconds) {
    if (!ok_) return;
    seconds=clampf(seconds,0,getDuration());
    auto* s=static_cast<ma_sound*>(sound_);
    ma_uint64 frames=0;
    ma_sound_get_length_in_pcm_frames(s,&frames);
    float duration=getDuration();
    ma_sound_seek_to_pcm_frame(s,duration>0?(ma_uint64)(seconds*(double)frames/duration):0);
}
void AudioClip::setPosition(float x,float y) {
    if(ok_) { auto* s=static_cast<ma_sound*>(sound_); ma_sound_set_spatialization_enabled(s,MA_TRUE); ma_sound_set_position(s,x,y,0); }
}
void AudioClip::setListenerPosition(float x,float y) {
    if(AudioEngine::instance().ok()) ma_engine_listener_set_position(audioEngineRaw(),0,x,y,0);
}
void AudioClip::setDistanceAttenuation(bool enable,float refDist,float maxDist) {
    if(!ok_) return;
    auto* s=static_cast<ma_sound*>(sound_);
    ma_sound_set_spatialization_enabled(s,enable?MA_TRUE:MA_FALSE);
    if(enable) {
        ma_sound_set_min_distance(s,std::max(0.f,refDist));
        ma_sound_set_max_distance(s,std::max(refDist,maxDist));
        ma_sound_set_attenuation_model(s,ma_attenuation_model_inverse);
    }
}

SoundPool::~SoundPool() {
    bool engineAlive = AudioEngine::instance().ok();   // 引擎已关则跳过 uninit, 防 UAF
    for (void* p : sounds_) {
        if (engineAlive) ma_sound_uninit(static_cast<ma_sound*>(p));
        delete static_cast<ma_sound*>(p);
    }
    sounds_.clear();
}
bool SoundPool::load(const char* path, int voices, AudioBus* bus) {
    if (!audioReady() || voices <= 0) return false;
    for (void* p : sounds_) {
        ma_sound_uninit(static_cast<ma_sound*>(p));
        delete static_cast<ma_sound*>(p);
    }
    sounds_.clear();
    startTimes_.clear();
    cur_ = 0;
    for (int i = 0; i < voices; ++i) {
        auto* s = new ma_sound;
        if (ma_sound_init_from_file(audioEngineRaw(), path, 0,
                                    bus ? static_cast<ma_sound_group*>(bus->handle()) : nullptr,
                                    nullptr, s) != MA_SUCCESS) {
            delete s;
            // 失败时释放已加载的全部声道, 防止泄漏
            for (void* p : sounds_) { ma_sound_uninit(static_cast<ma_sound*>(p)); delete static_cast<ma_sound*>(p); }
            sounds_.clear();
            startTimes_.clear();
            Log::error("[Audio] SoundPool 加载失败: %s", path);
            return false;
        }
        sounds_.push_back(s);
    }
    startTimes_.assign(sounds_.size(), 0.0);   // 各声道开始时间(用于抢占最旧)
    return true;
}
void SoundPool::play(float volume) {
    if (sounds_.empty()) return;
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    // 1) 优先找空闲声道
    for (size_t k = 0; k < sounds_.size(); ++k) {
        int i = (cur_ + (int)k) % (int)sounds_.size();
        auto* s = static_cast<ma_sound*>(sounds_[i]);
        if (!ma_sound_is_playing(s)) {
            cur_ = (i + 1) % (int)sounds_.size();
            ma_sound_set_volume(s, volume);
            ma_sound_seek_to_pcm_frame(s, 0);
            ma_sound_start(s);
            startTimes_[i] = now;
            return;
        }
    }
    // 2) 全部在播: 抢占开始时间最早(最旧)的声道
    int oldest = 0;
    for (int i = 1; i < (int)sounds_.size(); ++i)
        if (startTimes_[i] < startTimes_[oldest]) oldest = i;
    auto* s = static_cast<ma_sound*>(sounds_[oldest]);
    ma_sound_stop(s);
    ma_sound_set_volume(s, volume);
    ma_sound_seek_to_pcm_frame(s, 0);
    ma_sound_start(s);
    startTimes_[oldest] = now;
    cur_ = (oldest + 1) % (int)sounds_.size();
}
void SoundPool::stopAll() { for (void* p : sounds_) ma_sound_stop(static_cast<ma_sound*>(p)); }

MusicPlayer::~MusicPlayer() {
    if (sound_) {
        if (AudioEngine::instance().ok()) ma_sound_uninit(static_cast<ma_sound*>(sound_));
        delete static_cast<ma_sound*>(sound_);
        sound_ = nullptr;
    }
}
bool MusicPlayer::open(const char* path, AudioBus* bus) {
    if (!audioReady()) return false;
    auto* s = new ma_sound;
    if (ma_sound_init_from_file(audioEngineRaw(), path, MA_SOUND_FLAG_STREAM,
                                bus ? static_cast<ma_sound_group*>(bus->handle()) : nullptr,
                                nullptr, s) != MA_SUCCESS) {
        delete s;
        Log::error("[Audio] 音乐打开失败: %s", path);
        return false;
    }
    if (sound_) { ma_sound_uninit(static_cast<ma_sound*>(sound_)); delete static_cast<ma_sound*>(sound_); }
    sound_ = s; ok_ = true;
    return true;
}
void MusicPlayer::play()  { if (ok_) ma_sound_start(static_cast<ma_sound*>(sound_)); }
void MusicPlayer::pause() { if (ok_) ma_sound_stop(static_cast<ma_sound*>(sound_)); }  // 保持当前位置
void MusicPlayer::stop()  {
    if (!ok_) return;
    ma_sound_stop(static_cast<ma_sound*>(sound_));
    ma_sound_seek_to_pcm_frame(static_cast<ma_sound*>(sound_), 0);
}
void MusicPlayer::setLoop(bool loop) { if (ok_) ma_sound_set_looping(static_cast<ma_sound*>(sound_), loop); }
void MusicPlayer::setVolume(float v) { if (ok_) ma_sound_set_volume(static_cast<ma_sound*>(sound_), v); }
bool MusicPlayer::isPlaying() const { return ok_ && ma_sound_is_playing(static_cast<ma_sound*>(sound_)); }
float MusicPlayer::getPosition() const {
    float seconds=0;
    if(ok_) ma_sound_get_cursor_in_seconds(static_cast<ma_sound*>(sound_),&seconds);
    return seconds;
}
float MusicPlayer::getDuration() const {
    float seconds=0;
    if(ok_) ma_sound_get_length_in_seconds(static_cast<ma_sound*>(sound_),&seconds);
    return seconds;
}
void MusicPlayer::seek(float seconds) {
    if(!ok_) return;
    auto* s=static_cast<ma_sound*>(sound_);
    ma_uint64 frames=0;
    ma_sound_get_length_in_pcm_frames(s,&frames);
    float duration=getDuration();
    seconds=clampf(seconds,0,duration);
    ma_sound_seek_to_pcm_frame(s,duration>0?(ma_uint64)(seconds*(double)frames/duration):0);
}

// ============================== Input / InputMap ==============================
namespace {
struct InputState {
    GLFWwindow* win = nullptr;
    bool cur[GLFW_KEY_LAST + 1] = {}, prev[GLFW_KEY_LAST + 1] = {};
    bool mcur[GLFW_MOUSE_BUTTON_LAST + 1] = {}, mprev[GLFW_MOUSE_BUTTON_LAST + 1] = {};
    bool gpcur[GLFW_JOYSTICK_LAST + 1][GLFW_GAMEPAD_BUTTON_LAST + 1] = {};
    bool gpprev[GLFW_JOYSTICK_LAST + 1][GLFW_GAMEPAD_BUTTON_LAST + 1] = {};
    float axcur[GLFW_JOYSTICK_LAST + 1][GLFW_GAMEPAD_AXIS_LAST + 1] = {};
    float axprev[GLFW_JOYSTICK_LAST + 1][GLFW_GAMEPAD_AXIS_LAST + 1] = {};
    double mx = 0, my = 0;
    double scrollAccum = 0, scrollFrame = 0;   // 回调累积 -> 每帧结算一次
    static void scrollCb(GLFWwindow*, double, double yoff) { get().scrollAccum += yoff; }
    static InputState& get() { static InputState s; return s; }
};
}
void Input::update(Window& win) {
    auto& s = InputState::get();
    if (s.win != win.handle()) {          // 首次或换窗口时挂滚轮回调
        s.win = win.handle();
        if (s.win) glfwSetScrollCallback(s.win, &InputState::scrollCb);
    }
    std::memcpy(s.prev, s.cur, sizeof(s.cur));
    std::memcpy(s.mprev, s.mcur, sizeof(s.mcur));
    std::memcpy(s.gpprev, s.gpcur, sizeof(s.gpcur));
    std::memcpy(s.axprev, s.axcur, sizeof(s.axcur));
    for (int k = 0; k <= GLFW_KEY_LAST; ++k)
        s.cur[k] = s.win && glfwGetKey(s.win, k) == GLFW_PRESS;
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b)
        s.mcur[b] = s.win && glfwGetMouseButton(s.win, b) == GLFW_PRESS;
    for (int pad=0; pad<=GLFW_JOYSTICK_LAST; ++pad) {
        GLFWgamepadstate st{};
        bool connected = glfwGetGamepadState(pad,&st) == GLFW_TRUE;
        for(int b=0;b<=GLFW_GAMEPAD_BUTTON_LAST;++b) s.gpcur[pad][b]=connected && st.buttons[b]==GLFW_PRESS;
        for(int a=0;a<=GLFW_GAMEPAD_AXIS_LAST;++a) s.axcur[pad][a]=connected ? st.axes[a] : 0;
    }
    if (s.win) glfwGetCursorPos(s.win, &s.mx, &s.my);
    s.scrollFrame = s.scrollAccum;   // 本帧滚轮增量
    s.scrollAccum = 0;
}
bool Input::isKeyDown(int k)     { auto& s = InputState::get(); return k >= 0 && k <= GLFW_KEY_LAST && s.cur[k]; }
bool Input::isKeyPressed(int k)  { auto& s = InputState::get(); return k >= 0 && k <= GLFW_KEY_LAST && s.cur[k] && !s.prev[k]; }
bool Input::isKeyReleased(int k) { auto& s = InputState::get(); return k >= 0 && k <= GLFW_KEY_LAST && !s.cur[k] && s.prev[k]; }
bool Input::isMouseDown(int b)     { auto& s = InputState::get(); return b >= 0 && b <= GLFW_MOUSE_BUTTON_LAST && s.mcur[b]; }
bool Input::isMousePressed(int b)  { auto& s = InputState::get(); return b >= 0 && b <= GLFW_MOUSE_BUTTON_LAST && s.mcur[b] && !s.mprev[b]; }
bool Input::isMouseReleased(int b) { auto& s = InputState::get(); return b >= 0 && b <= GLFW_MOUSE_BUTTON_LAST && !s.mcur[b] && s.mprev[b]; }
Vec2 Input::mousePos() { auto& s = InputState::get(); return {(float)s.mx, (float)s.my}; }
float Input::scrollDelta() { return (float)InputState::get().scrollFrame; }
// ---- 手柄(基于 GLFW 手柄映射; 无手柄/未识别时安全返回 false/0) ----
bool Input::isGamepadConnected(int pad) {
    return glfwJoystickPresent(pad) && glfwJoystickIsGamepad(pad);
}
bool Input::isGamepadButtonDown(int pad, int btn) {
    GLFWgamepadstate st;
    if (!glfwGetGamepadState(pad, &st)) return false;
    return btn >= 0 && btn <= GLFW_GAMEPAD_BUTTON_LAST && st.buttons[btn] == GLFW_PRESS;
}
float Input::gamepadAxis(int pad, int axis) {
    GLFWgamepadstate st;
    if (!glfwGetGamepadState(pad, &st)) return 0;
    return (axis >= 0 && axis <= GLFW_GAMEPAD_AXIS_LAST) ? st.axes[axis] : 0;
}

void InputMap::bind(const char* action, int key) { if(action) binds_[action].push_back(key); }
void InputMap::unbind(const char* action, int key) {
    if (!action) return;
    auto it = binds_.find(action);
    if (it == binds_.end()) return;
    auto& v = it->second;
    v.erase(std::remove(v.begin(), v.end(), key), v.end());
}
void InputMap::clear(const char* action) {
    if(!action) return;
    binds_.erase(action); gamepadBinds_.erase(action); axisBinds_.erase(action); axisThresholds_.erase(action);
}
void InputMap::bindGamepad(const char* action,int button) {
    if(action && button>=0 && button<=GLFW_GAMEPAD_BUTTON_LAST) gamepadBinds_[action].push_back(button);
}
void InputMap::bindGamepadAxis(const char* action,int axis,float threshold) {
    if(!action || axis<0 || axis>GLFW_GAMEPAD_AXIS_LAST) return;
    axisBinds_[action].push_back(axis);
    axisThresholds_[action].push_back(clampf(std::fabs(threshold),0,1));
}
bool InputMap::down(const char* action) const {
    if(!action) return false;
    auto it = binds_.find(action);
    if (it != binds_.end()) for (int k : it->second) if (Input::isKeyDown(k)) return true;
    auto& s=InputState::get();
    auto gp=gamepadBinds_.find(action);
    if(gp!=gamepadBinds_.end()) {
        for(int pad=0;pad<=GLFW_JOYSTICK_LAST;++pad)
            for(int b:gp->second)
                if(b>=0 && b<=GLFW_GAMEPAD_BUTTON_LAST && s.gpcur[pad][b]) return true;
    }
    auto ax = axisBinds_.find(action);
    auto th = axisThresholds_.find(action);
    if(ax!=axisBinds_.end() && th!=axisThresholds_.end()) {
        for(int pad=0;pad<=GLFW_JOYSTICK_LAST;++pad)
            for(size_t i=0;i<ax->second.size() && i<th->second.size();++i) {
                int axis=ax->second[i];
                if(axis>=0 && axis<=GLFW_GAMEPAD_AXIS_LAST &&
                   std::fabs(s.axcur[pad][axis])>=th->second[i]) return true;
            }
    }
    return false;
}
bool InputMap::pressed(const char* action) const {
    if(!action) return false;
    auto it = binds_.find(action);
    if (it != binds_.end()) for (int k : it->second) if (Input::isKeyPressed(k)) return true;
    auto& s=InputState::get();
    auto gp=gamepadBinds_.find(action);
    if(gp!=gamepadBinds_.end()) {
        for(int pad=0;pad<=GLFW_JOYSTICK_LAST;++pad)
            for(int b:gp->second)
                if(b>=0 && b<=GLFW_GAMEPAD_BUTTON_LAST &&
                   s.gpcur[pad][b] && !s.gpprev[pad][b]) return true;
    }
    auto ax = axisBinds_.find(action);
    auto th = axisThresholds_.find(action);
    if(ax!=axisBinds_.end() && th!=axisThresholds_.end()) {
      for(int pad=0;pad<=GLFW_JOYSTICK_LAST;++pad)
        for(size_t i=0;i<ax->second.size() && i<th->second.size();++i) {
            int a=ax->second[i]; float t=th->second[i];
            if(a>=0 && a<=GLFW_GAMEPAD_AXIS_LAST &&
               std::fabs(s.axcur[pad][a])>=t && std::fabs(s.axprev[pad][a])<t) return true;
        }
    }
    return false;
}

std::string InputMap::serialize() const {
    Json root; root.type = Json::Object;
    for (const auto& kv : binds_) {
        Json arr; arr.type = Json::Array;
        for (int k : kv.second) { Json v; v.type = Json::Number; v.number = (double)k; arr.arr.push_back(v); }
        root.obj[kv.first] = arr;
    }
    return root.dump();
}
bool InputMap::deserialize(const std::string& data) {
    Json root;
    if (!Json::parse(data, root) || root.type != Json::Object) return false;
    binds_.clear();
    for (const auto& kv : root.obj) {
        if (kv.second.type != Json::Array) continue;
        for (const auto& v : kv.second.arr)
            if (v.type == Json::Number) binds_[kv.first].push_back((int)v.number);
    }
    return true;
}

// ============================== Scene / SceneManager ==============================
void SceneManager::add(const char* name, std::unique_ptr<Scene> s) {
    if (!name || !s) { Log::warn("[SceneManager] 忽略无效场景注册"); return; }
    auto it = scenes_.find(name);
    if (it != scenes_.end() && (it->second.get() == cur_ || it->second.get() == transPrev_)) {
        Log::warn("[SceneManager] 场景正在使用或过渡中，拒绝替换: %s", name);
        return;
    }
    scenes_[name] = std::move(s);
}
void SceneManager::remove(const char* name) {
    auto it = scenes_.find(name);
    if (it != scenes_.end() && it->second.get() != cur_ && it->second.get() != transPrev_ &&
        !(trans_ != Transition::None && it->first == transTarget_)) scenes_.erase(it);
}
void SceneManager::switchTo(const char* name) {
    if (!name) return;
    if (trans_ != Transition::None && transDuration_ > 0) {
        transPrev_ = cur_;
        transTarget_ = name; transTime_ = 0; transSwitched_ = false;
    } else pending_ = name;
}
void SceneManager::setTransition(Transition t,float duration) {
    trans_=t; transDuration_=std::max(0.f,duration); transTime_=0; transSwitched_=false;
    transTarget_.clear(); transPrev_=nullptr;
}
void SceneManager::update(float dt) {
    if (trans_ != Transition::None && transDuration_ > 0) {
        transTime_ += std::max(0.f,dt);
        if (!transSwitched_ && !transTarget_.empty() && transTime_ >= transDuration_*.5f) {
            pending_=transTarget_; transSwitched_=true;
        }
    }
    // 同帧内链式切换(A->B->C 一次完成), 上限 8 次防止 onEnter 循环切换
    int switchCount = 0;
    while (!pending_.empty() && switchCount < 8) {
        ++switchCount;
        auto it = scenes_.find(pending_);
        pending_.clear();
        if (it != scenes_.end() && it->second.get() != cur_) {
            if (trans_ != Transition::None && transDuration_ > 0 && !transPrev_) transPrev_ = cur_;
            if (cur_) cur_->onExit();
            cur_ = it->second.get();
            cur_->onEnter();         // 注意: onEnter 中可再次 switchTo, 由上限保护
        }
    }
    if (switchCount >= 8 && !pending_.empty()) {
        Log::warn("[SceneManager] 同帧切换超过 8 次, 可能形成循环, 停止切换");
        pending_.clear();
    }
    if (cur_) cur_->onUpdate(dt);
    if (trans_ != Transition::None && transDuration_ > 0 && transTime_ >= transDuration_) {
        trans_ = Transition::None;
        transTime_ = transDuration_ = 0;
        transTarget_.clear();
        transPrev_ = nullptr;
        transSwitched_ = false;
    }
}
void SceneManager::draw() {
    if (trans_ == Transition::None || transDuration_ <= 0) {
        if (cur_) cur_->onDraw();
        return;
    }
    auto& rs=detail::RS();
    float w=(float)rs.vw,h=(float)rs.vh, p=clampf(transTime_/transDuration_,0,1);
    if (trans_ == Transition::Slide) {
        Renderer::flush();
        const Camera2D* original=rs.cam;
        Camera2D baseCam=original ? *original : Camera2D{};
        if(!original) baseCam.pos={w*.5f,h*.5f};
        const Vec2 screenCenter{w*.5f,h*.5f};
        const Vec2 worldCenter=baseCam.screenToWorld(screenCenter,w,h);
        auto drawShifted = [&](Scene* scene,float contentOffset) {
            if (!scene) return;
            Camera2D slideCam=baseCam;
            Vec2 shiftedCenter=baseCam.screenToWorld(screenCenter+Vec2(-contentOffset,0),w,h);
            slideCam.pos += shiftedCenter-worldCenter;
            Renderer::setCamera(&slideCam);
            scene->onDraw();
            Renderer::flush();
        };
        auto nextIt=scenes_.find(transTarget_);
        Scene* nextScene=nextIt==scenes_.end()?cur_:nextIt->second.get();
        if (transPrev_) {
            float oldOffset=-w*std::min(2.f*p,1.f);
            float newOffset=p<.5f ? w : w*(2.f-2.f*p);
            drawShifted(transPrev_,oldOffset);
            if (p >= .5f) drawShifted(nextScene,newOffset);
        } else {
            drawShifted(nextScene,w*(1.f-p));
        }
        Renderer::setCamera(original);
        return;
    }
    if (cur_) cur_->onDraw();
    float alpha=p<.5f?p*2:(1-p)*2;
    Renderer::flush();
    const Camera2D* cam=rs.cam;
    Renderer::setCamera(nullptr);
    Renderer::drawRect(0,0,w,h,Color(0,0,0,alpha));
    Renderer::flush();
    Renderer::setCamera(cam);
}
Scene* SceneManager::get(const char* name) const {
    auto it = scenes_.find(name);
    return it == scenes_.end() ? nullptr : it->second.get();
}

// ============================== World(ECS) ==============================
Entity World::create() { Entity e = next_++; alive_.insert(e); return e; }
void World::destroy(Entity e) {
    if (!valid(e)) return;
    alive_.erase(e);
    for (auto& kv : pools_) kv.second->erase(e);
}
bool World::valid(Entity e) const { return alive_.count(e) != 0; }
void World::update(float dt) { for (auto& s : systems_) s->update(*this, dt); }
std::string World::serialize() const {
    Json root; root.type=Json::Object;
    Json entities; entities.type=Json::Array;
    for(Entity e:alive_) {
        Json entry; entry.type=Json::Object;
        Json id; id.type=Json::Number; id.number=e; entry.obj["id"]=id;
        Json comps; comps.type=Json::Object;
        for(const auto& named:componentNames_) {
            auto serializer=serializers_.find(named.first);
            if(serializer==serializers_.end()) continue;
            auto pool=pools_.find(named.first);
            if(pool==pools_.end() || !pool->second->contains(e)) continue;
            std::string text=serializer->second(*this,e);
            Json value; std::string err;
            if(!Json::parse(text,value,&err)) {
                Log::warn("[World] 组件 %s 序列化结果不是有效 JSON: %s；序列化被跳过，该组件不会写入存档",
                          named.second.c_str(),err.c_str());
                return {};
            }
            comps.obj[named.second]=std::move(value);
        }
        entry.obj["components"]=std::move(comps);
        entities.arr.push_back(std::move(entry));
    }
    root.obj["entities"]=std::move(entities);
    return root.dump();
}
bool World::deserialize(const std::string& data) {
    // deserialize 会清空现有实体和组件；组件内指针/引用可能悬空，请在加载后重新绑定。
    Json root; std::string err;
    if(!Json::parse(data,root,&err) || !root.isObject() || !root.has("entities") || !root["entities"].isArray()) {
        Log::error("[World] ECS JSON 解析失败: %s",err.empty()?"缺少 entities 数组":err.c_str()); return false;
    }
    std::unordered_set<Entity> ids;
    Entity maxId=0;
    const Json& entities=root["entities"];
    for(size_t i=0;i<entities.arr.size();++i) {
        const Json& entry=entities.arr[i];
        if(!entry.isObject() || !entry.has("id") || entry["id"].type!=Json::Number ||
           !entry.has("components") || !entry["components"].isObject()) {
            Log::error("[World] 实体数据格式无效(index=%zu)",i); return false;
        }
        double rawId=entry["id"].number;
        if(rawId<1 || rawId>UINT32_MAX || std::floor(rawId)!=rawId) {
            Log::error("[World] 实体 id 无效(index=%zu)",i); return false;
        }
        Entity id=(Entity)rawId;
        if(!ids.insert(id).second) { Log::error("[World] 重复实体 id: %u",id); return false; }
        maxId=std::max(maxId,id);
        for(const auto& comp:entry["components"].obj)
            if(!deserializers_.count(comp.first)) { Log::error("[World] 未注册组件类型: %s",comp.first.c_str()); return false; }
    }
    if(maxId==UINT32_MAX) { Log::error("[World] 实体 id 已达上限"); return false; }
    alive_.clear(); pools_.clear(); next_=maxId+1;
    for(size_t i=0;i<entities.arr.size();++i) {
        const Json& entry=entities.arr[i];
        Entity id=(Entity)entry["id"].asNumber();
        alive_.insert(id);
        for(const auto& comp:entry["components"].obj) deserializers_.at(comp.first)(*this,id,comp.second);
    }
    return true;
}

// ============================== ResourceManager ==============================
ResourceManager& ResourceManager::instance() { static ResourceManager m; return m; }
Texture* ResourceManager::texture(const char* path) {
    auto it = tex_.find(path);
    if (it != tex_.end()) return it->second.get();
    auto t = std::make_unique<Texture>();
    if (!t->load(path)) return nullptr;
    return tex_.emplace(path, std::move(t)).first->second.get();
}
Font* ResourceManager::font(const char* path, float size) {
    std::string key = std::string(path) + "@" + std::to_string((int)size);
    auto it = font_.find(key);
    if (it != font_.end()) return it->second.get();
    auto f = std::make_unique<Font>();
    if (!f->load(path, size)) return nullptr;
    return font_.emplace(key, std::move(f)).first->second.get();
}
AudioClip* ResourceManager::audioClip(const char* path) {
    auto it = clip_.find(path);
    if (it != clip_.end()) return it->second.get();
    auto c = std::make_unique<AudioClip>();
    if (!c->load(path)) return nullptr;
    return clip_.emplace(path, std::move(c)).first->second.get();
}
void ResourceManager::clear() { tex_.clear(); font_.clear(); clip_.clear(); }

// ============================== SaveSystem ==============================
bool SaveSystem::writeText(const char* path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (!f) { Log::error("[Save] 写入失败: %s", path); return false; }
    f << content;
    return true;
}
bool SaveSystem::readText(const char* path, std::string& out) {
    std::vector<unsigned char> bytes;
    if (!detail::readFileBytes(path, bytes)) return false;
    out.assign((const char*)bytes.data(), bytes.size());
    return true;
}
bool SaveSystem::writeBinary(const char* path, const void* data, size_t size) {
    std::ofstream f(path, std::ios::binary);
    if (!f) { Log::error("[Save] 写入失败: %s", path); return false; }
    f.write((const char*)data, (std::streamsize)size);
    return true;
}
bool SaveSystem::readBinary(const char* path, std::vector<unsigned char>& out) {
    return detail::readFileBytes(path, out);
}
bool SaveSystem::save(const char* path, const std::unordered_map<std::string, std::string>& kv) {
    std::string text;
    for (auto& p : kv) { text += p.first; text += '='; text += p.second; text += '\n'; }
    return writeText(path, text);
}
bool SaveSystem::load(const char* path, std::unordered_map<std::string, std::string>& kv) {
    std::string text;
    if (!readText(path, text)) return false;
    kv.clear();
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        kv[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return true;
}
int SaveSystem::getInt(const std::unordered_map<std::string, std::string>& kv, const char* key, int def) {
    auto it = kv.find(key); return it == kv.end() ? def : std::atoi(it->second.c_str());
}
float SaveSystem::getFloat(const std::unordered_map<std::string, std::string>& kv, const char* key, float def) {
    auto it = kv.find(key); return it == kv.end() ? def : (float)std::atof(it->second.c_str());
}
std::string SaveSystem::getStr(const std::unordered_map<std::string, std::string>& kv, const char* key, const char* def) {
    auto it = kv.find(key); return it == kv.end() ? std::string(def) : it->second;
}

// ============================== Json ==============================
const Json& Json::operator[](const char* key) const {
    static const Json nullJson;
    if (type != Object) return nullJson;
    auto it = obj.find(key);
    return it == obj.end() ? nullJson : it->second;
}
const Json& Json::at(size_t i) const {
    static const Json nullJson;
    return (type == Array && i < arr.size()) ? arr[i] : nullJson;
}

namespace {
struct JsonCur { const char* p; };
void jsonWs(JsonCur& c) { while (*c.p == ' ' || *c.p == '\t' || *c.p == '\n' || *c.p == '\r') ++c.p; }

bool jsonString(JsonCur& c, std::string& out) {
    if (*c.p != '"') return false;
    ++c.p; out.clear();
    while (*c.p && *c.p != '"') {
        if (*c.p == '\\') {
            ++c.p;
            switch (*c.p) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"';  break;
                case '\\': out += '\\'; break;
                case '/': out += '/';  break;
                case 'u': {          // \uXXXX -> UTF-8, 支持代理对
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = c.p[1 + i];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code += (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') code += (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code += (unsigned)(h - 'A' + 10);
                        else return false;
                    }
                    c.p += 4;
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        // 高代理: 必须紧跟 \uDC00~\uDFFF 低代理, 合并为完整码点
                        if (c.p[1] == '\\' && c.p[2] == 'u') {
                            unsigned low = 0; bool okLow = true;
                            for (int i = 0; i < 4; ++i) {
                                char h = c.p[3 + i];
                                low <<= 4;
                                if (h >= '0' && h <= '9') low += (unsigned)(h - '0');
                                else if (h >= 'a' && h <= 'f') low += (unsigned)(h - 'a' + 10);
                                else if (h >= 'A' && h <= 'F') low += (unsigned)(h - 'A' + 10);
                                else { okLow = false; break; }
                            }
                            if (okLow && low >= 0xDC00 && low <= 0xDFFF) {
                                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                                c.p += 6;   // 吞掉 "\uXXXX"
                            } else code = 0xFFFD;
                        } else code = 0xFFFD;
                    } else if (code >= 0xDC00 && code <= 0xDFFF) {
                        code = 0xFFFD;   // 孤立低代理
                    }
                    if (code < 0x80) out += (char)code;
                    else if (code < 0x800) { out += (char)(0xC0 | (code >> 6)); out += (char)(0x80 | (code & 0x3F)); }
                    else if (code < 0x10000) { out += (char)(0xE0 | (code >> 12)); out += (char)(0x80 | ((code >> 6) & 0x3F)); out += (char)(0x80 | (code & 0x3F)); }
                    else {   // 4 字节 UTF-8
                        out += (char)(0xF0 | (code >> 18));
                        out += (char)(0x80 | ((code >> 12) & 0x3F));
                        out += (char)(0x80 | ((code >> 6) & 0x3F));
                        out += (char)(0x80 | (code & 0x3F));
                    }
                    break;
                }
                default: return false;
            }
            if (*c.p) ++c.p;
        } else {
            out += *c.p++;           // UTF-8 原文直接透传
        }
    }
    if (*c.p != '"') return false;
    ++c.p;
    return true;
}

bool jsonValue(JsonCur& c, Json& out);

bool jsonArray(JsonCur& c, Json& out) {
    ++c.p;                            // '['
    out.type = Json::Array;
    jsonWs(c);
    if (*c.p == ']') { ++c.p; return true; }
    while (true) {
        Json v;
        if (!jsonValue(c, v)) return false;
        out.arr.push_back(std::move(v));
        jsonWs(c);
        if (*c.p == ',') { ++c.p; continue; }
        if (*c.p == ']') { ++c.p; return true; }
        return false;
    }
}

bool jsonObject(JsonCur& c, Json& out) {
    ++c.p;                            // '{'
    out.type = Json::Object;
    jsonWs(c);
    if (*c.p == '}') { ++c.p; return true; }
    while (true) {
        jsonWs(c);
        std::string key;
        if (!jsonString(c, key)) return false;
        jsonWs(c);
        if (*c.p != ':') return false;
        ++c.p;
        jsonWs(c);
        Json v;
        if (!jsonValue(c, v)) return false;
        out.obj[key] = std::move(v);
        jsonWs(c);
        if (*c.p == ',') { ++c.p; continue; }
        if (*c.p == '}') { ++c.p; return true; }
        return false;
    }
}

bool jsonValue(JsonCur& c, Json& out) {
    jsonWs(c);
    switch (*c.p) {
        case '{': return jsonObject(c, out);
        case '[': return jsonArray(c, out);
        case '"': out.type = Json::String; return jsonString(c, out.str);
        case 't': if (std::strncmp(c.p, "true", 4) == 0)  { c.p += 4; out.type = Json::Bool; out.boolean = true; return true; } return false;
        case 'f': if (std::strncmp(c.p, "false", 5) == 0) { c.p += 5; out.type = Json::Bool; out.boolean = false; return true; } return false;
        case 'n': if (std::strncmp(c.p, "null", 4) == 0)  { c.p += 4; out.type = Json::Null; return true; } return false;
        default: {
            char* end = nullptr;
            double v = std::strtod(c.p, &end);
            if (end == c.p) return false;
            c.p = end;
            out.type = Json::Number;
            out.number = v;
            return true;
        }
    }
}

void jsonEscape(const std::string& s, std::string& out) {
    for (char ch : s) {
        switch (ch) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\r': out += "\\r";  break;
            default:   out += ch;     break;
        }
    }
}
} // namespace

bool Json::parse(const std::string& text, Json& out, std::string* err) {
    const char* p = text.c_str();
    // 跳过 UTF-8 BOM(EF BB BF)
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
    JsonCur c{p};
    if (!jsonValue(c, out)) { if (err) *err = "JSON 解析失败"; return false; }
    jsonWs(c);
    if (*c.p != 0) { if (err) *err = "JSON 末尾存在多余字符"; return false; }
    return true;
}
bool Json::load(const char* path, Json& out) {
    std::string text;
    if (!SaveSystem::readText(path, text)) { Log::error("[Json] 读取失败: %s", path); return false; }
    std::string err;
    if (!parse(text, out, &err)) { Log::error("[Json] %s: %s", path, err.c_str()); return false; }
    return true;
}
std::string Json::dump(int indent) const {
    std::string pad((size_t)indent, ' '), pad2((size_t)indent + 2, ' '), out;
    switch (type) {
        case Null: return "null";
        case Bool: return boolean ? "true" : "false";
        case Number: {   // 整数值走 %lld 保精度, 其余用 %.17g 防丢位
            char b[40];
            if (number == (double)(long long)number && number >= -9007199254740992.0 && number <= 9007199254740992.0)
                std::snprintf(b, sizeof(b), "%lld", (long long)number);
            else
                std::snprintf(b, sizeof(b), "%.17g", number);
            return b;
        }
        case String: { std::string e = "\""; jsonEscape(str, e); e += '"'; return e; }
        case Array: {
            if (arr.empty()) return "[]";
            out = "[\n";
            for (size_t i = 0; i < arr.size(); ++i)
                out += pad2 + arr[i].dump(indent + 2) + (i + 1 < arr.size() ? ",\n" : "\n");
            return out + pad + "]";
        }
        case Object: {
            if (obj.empty()) return "{}";
            out = "{\n";
            size_t i = 0;
            for (auto& kv : obj) {
                std::string key = "\"";
                jsonEscape(kv.first, key);
                out += pad2 + key + "\": " + kv.second.dump(indent + 2) + (++i < obj.size() ? ",\n" : "\n");
            }
            return out + pad + "}";
        }
    }
    return "null";
}

// ============================== 玩法: 背包 / 交互 / 对话 / 任务 ==============================
std::unordered_map<std::string, Item>& itemRegistry() {
    static std::unordered_map<std::string, Item> reg;
    return reg;
}
void registerItem(const Item& item) { itemRegistry()[item.id] = item; }
const Item* findItem(const std::string& id) {
    auto it = itemRegistry().find(id);
    return it == itemRegistry().end() ? nullptr : &it->second;
}

bool Interactable::inRange(Vec2 playerPos, Vec2 playerSize) const {
    if (!enabled) return false;
    Rect a{position.x, position.y, size.x, size.y};
    Rect b{playerPos.x, playerPos.y, playerSize.x, playerSize.y};
    return a.intersects(b) || pointInRect(playerPos, a);
}
void PickupInteractable::onInteract() {
    if (picked) return;
    const Item* it = findItem(itemId);
    if (!inventory || !it) return;
    int got = inventory->add(*it, amount);
    if (got > 0) { picked = true; Log::info("拾取 %s x%d", it->name.c_str(), got); }
}

void Inventory::resize(int capacity) {
    capacity_ = capacity < 0 ? 0 : capacity;
    slots_.assign((size_t)capacity_, ItemStack{});
}
int Inventory::add(const Item& item, int count) {
    if (count <= 0) return 0;
    int remaining = count, added = 0;
    for (auto& s : slots_) {   // 先堆叠到已有同类且未满的格子
        if (remaining <= 0) break;
        if (s.count > 0 && s.item.id == item.id && s.count < s.item.maxStack) {
            int space = s.item.maxStack - s.count;
            int put = space < remaining ? space : remaining;
            s.count += put; remaining -= put; added += put;
        }
    }
    for (auto& s : slots_) {   // 再开新格
        if (remaining <= 0) break;
        if (s.count == 0) {
            s.item = item;
            int put = item.maxStack < remaining ? item.maxStack : remaining;
            s.count = put; remaining -= put; added += put;
        }
    }
    if (remaining > 0) Log::warn("[Inventory] 背包已满, 丢弃 %d 个 %s", remaining, item.name.c_str());
    return added;
}
int Inventory::remove(const std::string& id, int count) {
    int remaining = count, removed = 0;
    for (auto& s : slots_) {
        if (remaining <= 0) break;
        if (s.count > 0 && s.item.id == id) {
            int take = s.count < remaining ? s.count : remaining;
            s.count -= take; remaining -= take; removed += take;
            if (s.count == 0) s.item = Item{};   // 清空该格
        }
    }
    return removed;
}
int Inventory::count(const std::string& id) const {
    int total = 0;
    for (auto& s : slots_) if (s.count > 0 && s.item.id == id) total += s.count;
    return total;
}
bool Inventory::has(const std::string& id, int count) const { return count <= 0 || this->count(id) >= count; }
bool Inventory::isFull() const {
    for (auto& s : slots_) if (s.count == 0) return false;
    return true;
}
void Inventory::clear() { slots_.assign((size_t)capacity_, ItemStack{}); }
std::string Inventory::serialize() const {
    Json arr; arr.type = Json::Array;
    for (auto& s : slots_) {
        if (s.count <= 0) continue;
        Json o; o.type = Json::Object;
        Json id; id.type = Json::String; id.str = s.item.id;
        Json c;  c.type  = Json::Number; c.number = s.count;
        o.obj["id"] = id; o.obj["count"] = c;
        arr.arr.push_back(o);
    }
    return arr.dump();
}
bool Inventory::deserialize(const std::string& data) {
    Json arr;
    if (!Json::parse(data, arr) || !arr.isArray()) return false;
    clear();
    for (auto& e : arr.arr) {
        const Item* it = findItem(e["id"].str);
        if (!it) continue;
        int n = e["count"].asInt(1);
        add(*it, n);
    }
    return true;
}

void Dialog::clear() { nodes_.clear(); current_ = 0; finished_ = true; }
int Dialog::addNode(const DialogNode& n) { nodes_.push_back(n); return (int)nodes_.size() - 1; }
void Dialog::startAt(int node) {
    current_ = (node >= 0 && (size_t)node < nodes_.size()) ? node : 0;
    finished_ = nodes_.empty();
}
const DialogNode* Dialog::current() const {
    return (!finished_ && current_ >= 0 && (size_t)current_ < nodes_.size()) ? &nodes_[current_] : nullptr;
}
bool Dialog::choose(int option) {
    const DialogNode* n = current();
    if (!n) return false;
    if (option < 0 || (size_t)option >= n->next.size()) return false;
    int target = n->next[option];
    if (target < 0 || (size_t)target >= nodes_.size()) { finished_ = true; return true; }
    current_ = target; return true;
}
void Dialog::advance() {
    const DialogNode* n = current();
    if (!n) { finished_ = true; return; }
    if (!n->next.empty()) return;   // 有选项分支时不自动推进(等待 choose)
    if (current_ + 1 < (int)nodes_.size()) ++current_;   // 无分支: 顺序推进
    else finished_ = true;
}
bool Dialog::load(const Json& data) {
    // 期望格式: { "nodes": [ {speaker, text, choices:[], next:[]} ] }
    const Json& arr = data["nodes"];
    if (!arr.isArray()) return false;
    clear();
    for (auto& e : arr.arr) {
        DialogNode n;
        n.speaker = e["speaker"].str;
        n.text = e["text"].str;
        const Json& ch = e["choices"];
        if (ch.isArray()) for (auto& c : ch.arr) n.choices.push_back(c.str);
        const Json& nx = e["next"];
        if (nx.isArray()) for (auto& c : nx.arr) n.next.push_back(c.asInt(-1));
        addNode(n);
    }
    return true;
}

void Quest::start()   { if (state == State::NotStarted) { state = State::InProgress; progress = 0; } }
void Quest::advance(int n) {
    if (state != State::InProgress) return;
    progress += n;
    if (progress >= required) { progress = required; state = State::Completed; }
}
void Quest::complete() { state = State::Completed; progress = required; }
void Quest::reset()    { state = State::NotStarted; progress = 0; }

// ============================== EventBus ==============================
EventBus& EventBus::instance() { static EventBus b; return b; }
void EventBus::unsubscribe(int id) {
    auto it = idToType_.find(id);          // O(1) 定位事件类型
    if (it == idToType_.end()) return;
    auto sit = subs_.find(it->second);
    if (sit != subs_.end()) {
        auto& v = sit->second;
        v.erase(std::remove_if(v.begin(), v.end(), [id](const Sub& s) { return s.id == id; }), v.end());
    }
    idToType_.erase(it);
}

// ============================== Profiler ==============================
Profiler& Profiler::instance() { static Profiler p; return p; }
Profiler::Scope::~Scope() {
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0_).count();
    if (p_) p_->_add(name_, ms);
}
void Profiler::_add(const char* name, double ms) {
    Stat& s = stats_[name];
    s.totalMs += ms;
    if (ms > s.maxMs) s.maxMs = ms;
    s.lastMs = ms;
    s.recent.push_back(ms);
    if (s.recent.size() > 60) s.recent.erase(s.recent.begin());
    s.recentMs = 0;
    for (double value : s.recent) s.recentMs += value;
    s.recentMs = s.recent.empty() ? 0 : s.recentMs / s.recent.size();
    ++s.count;
}
void Profiler::reset() { stats_.clear(); }
void Profiler::print() {
    Log::info("---- Profiler ----");
    for (auto& kv : stats_) {
        const Stat& s = kv.second;
        Log::info("  %-24s 次数:%-6ld 总计:%8.2fms 平均:%7.3fms 最近:%7.3fms 峰值:%7.3fms",
                  kv.first.c_str(), s.count, s.totalMs, s.count ? s.totalMs / s.count : 0, s.recentMs, s.maxMs);
    }
}

// ============================== GameLoop ==============================
int GameLoop::run(Window& win, UpdateFn update, RenderFn render, int fps) {
    const bool capped = fps > 0;
    const float fixed = capped ? 1.f / (float)fps : 0.f;
    float acc = 0;
    running_ = true;
    while (running_ && !win.shouldClose()) {
        auto frameStart = std::chrono::steady_clock::now();
        win.pollEvents();
        int w, h;
        win.getSize(w, h);
        Renderer::setViewport(w, h);
        Input::update(win);
        Time::_tick();
        if (capped) {
            acc += Time::delta();
            int steps = 0;
            while (acc >= fixed && steps < 5) {
                if (update) update(fixed);
                acc -= fixed;
                ++steps;
            }
            if (steps == 5) acc = 0;
        } else {
            if (update) update(Time::delta());
        }
        Renderer::resetStats();
        if (render) render();
        Renderer::flush();
        win.swapBuffers();
        if (capped) {
            auto deadline=frameStart+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(fixed));
            std::this_thread::sleep_until(deadline);
        }
    }
    return 0;
}
void GameLoop::stop() { running_ = false; }

// ============================== App ==============================
int App::run() {
    Window win;
    if (!win.create(width, height, title, vsync)) return 1;
    win_ = &win;                       // window() 在运行期间可访问底层窗口
    if (!gl::init()) { win_ = nullptr; win.destroy(); return 2; }
    if (!Renderer::init()) { win_ = nullptr; win.destroy(); return 3; }
    if (audio) AudioEngine::instance().init();
    if (setup) setup();
    GameLoop loop;
    loop.run(win,
             [&](float dt) { if (audio) AudioEngine::instance().updateFades(dt); if (update) update(dt); },
             [&] { Renderer::clear(clearColor); if (draw) draw(); },
             targetFps);
    win_ = nullptr;
    Renderer::shutdown();
    if (audio) AudioEngine::instance().shutdown();
    win.destroy();
    glfwTerminate();
    return 0;
}

// ============================== 工具函数 ==============================
float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float randf(float lo, float hi) {
    static bool seeded = [] {
        std::srand((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
        return true;
    }();
    (void)seeded;
    return lo + (hi - lo) * ((float)std::rand() / ((float)RAND_MAX + 1.f));
}
int randi(int lo, int hi) { (void)randf(0, 1); return lo + std::rand() % (hi - lo + 1); }

} // namespace lgnite

#endif // LGNITE_IMPLEMENTATION

// ############################################################################
// #################### 分阶段最小示例(可直接编译运行) ########################
// ############################################################################
//
// 【S0 入门图形】模块: App / Renderer / Color
// 证明: 10 行出画面, 会画矩形/圆/线
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// int main() {
//     App app;
//     app.title = "S0 几何画板";
//     app.draw = [] {
//         Renderer::drawRect(100, 100, 200, 120, Color::Orange);
//         Renderer::drawCircle(500, 200, 60, Color::Cyan);
//         Renderer::drawLine(100, 400, 700, 300, 4, Color::Yellow);
//     };
//     return app.run();
// }
//
// 【S1 基础游戏】贪吃蛇核心。模块: App / Renderer / Input / Rect 碰撞
// 证明: 网格移动 + 食物 + 碰撞 + 计分, 只用基础 API 即可成游戏
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// int main() {
//     const int N = 20, T = 24;                 // 网格数 / 每格像素
//     std::vector<Vec2> snake{{5, 5}};
//     Vec2 dir{1, 0}, food{10, 10};
//     float timer = 0; int score = 0; bool dead = false;
//     App app; app.title = "S1 贪吃蛇";
//     auto spawnFood = [&] {              // 重试直到不在蛇身上(上限 100 次)
//         for (int tries = 0; tries < 100; ++tries) {
//             Vec2 f{(float)randi(0, N-1), (float)randi(0, N-1)};
//             bool onSnake = false;
//             for (auto& s : snake) if (s == f) { onSnake = true; break; }
//             if (!onSnake) { food = f; return; }
//         }
//     };
//     app.update = [&](float dt) {
//         if (dead) { if (Input::isKeyPressed(GLFW_KEY_R)) { snake = {{5,5}}; dir={1,0}; score=0; dead=false; spawnFood(); } return; }
//         if (Input::isKeyPressed(GLFW_KEY_W) && dir.y != 1)  dir = {0, -1};
//         if (Input::isKeyPressed(GLFW_KEY_S) && dir.y != -1) dir = {0, 1};
//         if (Input::isKeyPressed(GLFW_KEY_A) && dir.x != 1)  dir = {-1, 0};
//         if (Input::isKeyPressed(GLFW_KEY_D) && dir.x != -1) dir = {1, 0};
//         timer += dt;
//         if (timer < 0.15f) return;
//         timer = 0;
//         Vec2 head = snake[0] + dir;
//         if (head.x < 0 || head.x >= N || head.y < 0 || head.y >= N) dead = true;      // 撞墙
//         for (auto& s : snake) if (s == head) dead = true;                              // 撞自己
//         if (dead) return;
//         snake.insert(snake.begin(), head);
//         if (head == food) { score += 10; spawnFood(); }
//         else snake.pop_back();
//         char buf[64]; snprintf(buf, sizeof(buf), "S1 贪吃蛇  分数 %d (R 重开)", score);
//         app.window()->setTitle(buf);                    // 分数显示在标题, 不依赖 Font
//     };
//     app.draw = [&] {
//         for (auto& s : snake) Renderer::drawRect(s.x*T+1, s.y*T+1, T-2, T-2, Color::Green);
//         Renderer::drawRect(food.x*T+2, food.y*T+2, T-4, T-4, Color::Red);
//     };
//     app.width = app.height = N * T;
//     return app.run();
// }
//
// 【S2 小型游戏】飞机大战骨架。模块: Sprite / Animation / SoundPool / Font / Scene / SceneManager
// 证明: 精灵动画 + 音效池 + 文字计分 + 场景切换, 可扩展成完整 STG
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// Texture gPlane; Sprite gPlayer; SoundPool gShoot; Font gFont; int gScore = 0;
// bool gHasRes = true;
// SceneManager gScenes;
// struct Fly { Sprite s; float vy; };                                // 敌机/子弹共用的小结构
// class PlayScene : public Scene {
//     float spawn = 0;
//     std::vector<Fly> enemies, bullets;
// public:
//     void onEnter() override { gScore = 0; enemies.clear(); bullets.clear(); }
//     void onUpdate(float dt) override {
//         if (Input::isKeyDown(GLFW_KEY_LEFT))  gPlayer.pos.x -= 300 * dt;
//         if (Input::isKeyDown(GLFW_KEY_RIGHT)) gPlayer.pos.x += 300 * dt;
//         if (Input::isKeyPressed(GLFW_KEY_SPACE)) {                     // 开火
//             gShoot.play();
//             Fly b; b.s.pos = gPlayer.pos; b.vy = -500.f;               // 子弹向上
//             bullets.push_back(b);
//         }
//         spawn += dt;                                                   // 敌机生成: 每 0.8 秒一架
//         if (spawn > 0.8f) {
//             spawn = 0;
//             Fly e; e.s.tex = gPlane.valid() ? &gPlane : nullptr;       // 无贴图时画占位矩形
//             e.s.autoSize = false; e.s.size = {32, 32};                 // 占位/贴图统一 32x32, bounds 一致
//             e.s.pos = {randf(0, 760), -40}; e.vy = 120.f;              // 敌机向下
//             enemies.push_back(e);
//         }
//         for (auto& b : bullets) b.s.pos.y += b.vy * dt;
//         for (auto& e : enemies) e.s.pos.y += e.vy * dt;
//         // 子弹 <-> 敌机碰撞
//         for (auto bi = bullets.begin(); bi != bullets.end();) {
//             bool hit = false;
//             for (auto ei = enemies.begin(); ei != enemies.end();) {
//                 if (bi->s.bounds().intersects(ei->s.bounds())) { hit = true; gScore += 10; ei = enemies.erase(ei); }
//                 else ++ei;
//             }
//             bi = hit ? bullets.erase(bi) : ++bi;
//         }
//         enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
//                                      [](const Fly& e) { return e.s.pos.y > 620; }), enemies.end());
//         bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
//                                      [](const Fly& b) { return b.s.pos.y < -20; }), bullets.end());
//         // 分数: 有 Font 画在屏幕(见 onDraw), 无 Font 时由 main 每帧写到窗口标题
//     }
//     void onDraw() override {
//         for (auto& e : enemies) {
//             if (e.s.tex) e.s.draw();
//             else Renderer::drawRect(e.s.pos.x, e.s.pos.y, 32, 32, Color::Red);   // 占位敌机
//         }
//         for (auto& b : bullets) Renderer::drawRect(b.s.pos.x + 14, b.s.pos.y, 4, 12, Color::Yellow);
//         if (gPlayer.tex) gPlayer.draw();
//         else Renderer::drawRect(gPlayer.pos.x, gPlayer.pos.y, 32, 32, Color::Blue);  // 占位玩家
//         if (gFont.valid()) {
//             char buf[32]; snprintf(buf, sizeof(buf), "SCORE %d", gScore);
//             gFont.drawText(10, 10, buf, Color::White, 1.2f);
//         }
//     }
// };
// int main() {
//     App app; app.title = "S2 飞机大战";
//     app.setup = [&] {
//         if (!gPlane.load("plane.png")) { Log::error("[进阶] plane.png 缺失, 用占位矩形"); gHasRes = false; }
//         if (!gShoot.load("shoot.wav", 8)) Log::error("[进阶] shoot.wav 缺失, 无开火音效");
//         if (!gFont.load("font.ttf", 20)) Log::error("[进阶] font.ttf 缺失, 分数改显示在标题");
//         if (!gHasRes) app.title = "S2 飞机大战 (资源缺失, 占位模式)";
//         gPlayer.tex = gPlane.valid() ? &gPlane : nullptr; gPlayer.pos = {380, 500};
//         gScenes.add("play", std::make_unique<PlayScene>());
//         gScenes.switchTo("play");       // 另有 menu/gameover 场景同理注册
//     };
//     app.update = [&](float dt) {
//         gScenes.update(dt);
//         if (!gFont.valid()) {                                          // 无字体: 分数走标题
//             char buf[64]; snprintf(buf, sizeof(buf), "S2 飞机大战  SCORE %d", gScore);
//             app.window()->setTitle(buf);
//         }
//     };
//     app.draw   = [] { gScenes.draw(); };
//     return app.run();
// }
//
// 【S3 中型游戏】平台跳跃骨架。模块: Tilemap / Camera2D 跟随 / ParticleSystem / SaveSystem
// 证明: 瓦片地图 + 摄像机跟随 + 粒子 + 存档, 即一个小型平台跳跃/RPG 雏形
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// Tilemap gMap; Texture gTiles; Camera2D gCam; ParticleSystem gFx;
// Vec2 gPlayer{64, 64}; float gVy = 0; bool gOnGround = false;
// int main() {
//     App app; app.title = "S3 平台跳跃"; app.width = 960; app.height = 540;
//     app.setup = [] {
//         if (!gTiles.load("tiles.png")) Log::error("[进阶] tiles.png 缺失, 地面用占位矩形画在 onDraw");
//         gMap.create(100, 20, 32);
//         if (gTiles.valid()) gMap.setTileset(gTiles, 8);
//         for (int x = 0; x < 100; ++x) gMap.set(x, 15, 1);              // 地面
//         gCam.clampEnabled = true; gCam.clampRect = {0, 0, 100 * 32.f, 20 * 32.f};
//         gFx.color0 = Color::Yellow; gFx.emitting = false;              // 落地/拾取粒子
//         std::unordered_map<std::string, std::string> kv;
//         if (SaveSystem::load("save.ini", kv)) {                        // 读档: 位置 x/y
//             gPlayer.x = SaveSystem::getFloat(kv, "px", 64);
//             gPlayer.y = SaveSystem::getFloat(kv, "py", 64);
//         }
//     };
//     app.update = [](float dt) {
//         if (Input::isKeyDown(GLFW_KEY_A)) gPlayer.x -= 200 * dt;
//         if (Input::isKeyDown(GLFW_KEY_D)) gPlayer.x += 200 * dt;
//         if (Input::isKeyPressed(GLFW_KEY_SPACE) && gOnGround) { gVy = -500.f; gOnGround = false; }
//         gVy += 1200.f * dt; gPlayer.y += gVy * dt;                     // 重力 + 下落
//         gOnGround = false;
//         Rect pb{gPlayer.x, gPlayer.y, 24, 32};                         // 与瓦片 AABB 碰撞
//         for (int ty = 0; ty < gMap.height(); ++ty)
//             for (int tx = 0; tx < gMap.width(); ++tx) {
//                 if (gMap.get(tx, ty) < 0) continue;
//                 Rect tb = gMap.tileBounds(tx, ty);
//                 if (!pb.intersects(tb)) continue;
//                 if (gVy > 0) { gPlayer.y = tb.y - 32; gVy = 0; gOnGround = true; }
//                 else         { gPlayer.y = tb.y + tb.h; gVy = 0; }
//                 pb.y = gPlayer.y;
//             }
//         gCam.follow(gPlayer, dt, 960, 540);
//         gFx.update(dt);
//         gFx.pos = gPlayer + Vec2{12, 16};                              // 粒子从玩家中心发
//         if (Input::isKeyPressed(GLFW_KEY_E)) gFx.emit(12);             // 按 E 放粒子
//         if (Input::isKeyPressed(GLFW_KEY_F5)) {                        // 快速存档(x 和 y)
//             SaveSystem::save("save.ini", {{"px", std::to_string(gPlayer.x)},
//                                           {"py", std::to_string(gPlayer.y)}});
//         }
//     };
//     app.draw = [] {
//         Renderer::setCamera(&gCam);
//         gMap.draw();
//         if (!gTiles.valid())                                           // 无贴图: 地面画占位灰条
//             for (int x = 0; x < gMap.width(); ++x)
//                 Renderer::drawRect(x * 32.f, 15 * 32.f, 32, 32, Color::Gray);
//         Renderer::drawRect(gPlayer.x, gPlayer.y, 24, 32, Color::Cyan);
//         gFx.draw();
//         Renderer::setCamera(nullptr);                                  // 回到屏幕坐标画 UI
//     };
//     return app.run();
// }
//
// 【S4 大型游戏】骨架。模块: ECS(World/System) / ObjectPool / EventBus / Profiler / Config
// 证明: 数据驱动 + 系统解耦 + 高频对象复用, 支撑大型 2D 原型
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// struct Velocity { Vec2 v; };
// struct HitEvent { Entity e; int damage; };                            // 事件类型
// struct Bullet { Vec2 pos; float life = 0; };                          // 子弹: 位置 + 寿命
// ObjectPool<Bullet> gBullets(256);                                     // 子弹池(复用, 不频繁 new)
// struct MoveSys : System {
//     void update(World& w, float dt) override {
//         LGNITE_PROFILE("MoveSys");
//         w.each<Transform, Velocity>([dt](Entity, Transform& t, Velocity& v) { t.pos += v.v * dt; });
//     }
// };
// struct RenderSys : System {
//     void update(World& w, float) override {
//         LGNITE_PROFILE("RenderSys");
//         w.each<Transform>([](Entity, Transform& t) {
//             Renderer::drawRect(t.pos.x, t.pos.y, 16 * t.scale.x, 16 * t.scale.y, Color::Green, t.rotation);
//         });
//     }
// };
// int main() {
//     App app; app.title = "S4 ECS 骨架";
//     World world;
//     app.setup = [&] {
//         Config cfg;                                                    // 数据驱动配置
//         if (Config::load("game.json", cfg)) Log::info("初始生命: %g", cfg["player"]["hp"].asNumber(3));
//         else Log::warn("[扩展] game.json 不存在, 使用默认配置");        // 无配置也能跑
//         EventBus::instance().subscribe<HitEvent>([](const HitEvent& e) {
//             Log::info("实体 %u 受击 %d 点", e.e, e.damage);            // 事件日志在这里刷
//         });
//         world.addSystem<MoveSys>();
//         world.addSystem<RenderSys>();
//         for (int i = 0; i < 500; ++i) {
//             Entity e = world.create();
//             world.add<Transform>(e).pos = {randf(0, 780), randf(0, 580)};
//             world.add<Velocity>(e).v = {randf(-80, 80), randf(-80, 80)};
//         }
//     };
//     app.update = [&](float dt) {
//         world.update(dt);                                              // 跑全部系统
//         {   // ---- 子弹池: 随机发射 -> 上移 -> 过期回收 ----
//             LGNITE_PROFILE("Bullets");
//             if (randf(0, 1) < dt * 10) {                               // 每秒约 10 发
//                 Bullet* b = gBullets.acquire();
//                 if (b) { b->pos = {randf(0, 780), 600}; b->life = 2.f; }
//             }
//             gBullets.forEach([dt](Bullet& b) { b.pos.y -= 300 * dt; b.life -= dt; });
//             std::vector<Bullet*> dead;                                 // 先收集再回收(避免遍历时修改)
//             gBullets.forEach([&](Bullet& b) { if (b.life <= 0) dead.push_back(&b); });
//             for (Bullet* b : dead) gBullets.release(b);
//         }
//         if (randf(0, 1) < dt)                                          // 每秒约 1 次随机受击事件
//             EventBus::instance().emit(HitEvent{world.create(), 10});
//         if (Input::isKeyPressed(GLFW_KEY_P)) Profiler::instance().print();
//     };
//     app.draw = [] {                                                    // 画子弹(小矩形)
//         gBullets.forEach([](Bullet& b) { Renderer::drawRect(b.pos.x, b.pos.y, 4, 10, Color::Orange); });
//     };
//     return app.run();
// }
//
// 【补充】RenderTarget 离屏渲染 + 后处理(整屏滤镜)
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// int main() {
//     App app; app.title = "RenderTarget 后处理"; app.width = 800; app.height = 600;
//     RenderTarget rt; Texture gTiles; Tilemap map; Camera2D cam; float t = 0;
//     app.setup = [&] {
//         rt.create(800, 600);                                  // 创建离屏目标(FBO)
//         if (!gTiles.load("tiles.png")) Log::error("tiles.png 缺失(地面用占位块)");
//         map.create(40, 24, 32); if (gTiles.valid()) map.setTileset(gTiles, 8);
//         for (int x = 0; x < 40; ++x) { map.set(x, 22, 1); map.set(x, 23, 1); }
//         cam.pos = {640, 360};
//     };
//     app.update = [&](float dt) { t += dt; cam.pos.x = 640 + std::sin(t) * 80.f; };
//     app.draw = [&] {
//         rt.bind();                                            // 1) 场景画到离屏
//         Renderer::setCamera(&cam); map.draw(); Renderer::setCamera(nullptr);
//         rt.unbind();
//         // 2) 离屏纹理带 tint 画回屏幕 —— 这里演示"半透明青色滤镜"
//         Renderer::drawTexture(rt.texture(), 0, 0, 800, 600, Color{0.8f, 1, 1, 0.95f});
//     };
//     return app.run();
// }
//
// 【补充】InputMap 动作映射 + 手柄: 键盘/手柄统一成"动作", 支持改键
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// int main() {
//     App app; app.title = "InputMap 动作映射";
//     InputMap im; Vec2 p{400, 300};
//     app.setup = [&] {
//         im.bind("left",  GLFW_KEY_A);   im.bind("left",  GLFW_KEY_LEFT);   // 动作可绑多键
//         im.bind("right", GLFW_KEY_D);   im.bind("right", GLFW_KEY_RIGHT);
//         im.bind("up",    GLFW_KEY_W);   im.bind("up",    GLFW_KEY_UP);
//         im.bind("down",  GLFW_KEY_S);   im.bind("down",  GLFW_KEY_DOWN);
//         im.bindGamepad("jump", GLFW_GAMEPAD_BUTTON_A);     // 手柄 A 键
//     };
//     app.update = [&](float dt) {
//         float sp = 200.f * dt;
//         if (im.down("left"))  p.x -= sp;                    // 按住
//         if (im.down("right")) p.x += sp;
//         if (im.down("up"))    p.y -= sp;
//         if (im.down("down"))  p.y += sp;
//         if (im.pressed("jump")) Log::info("手柄 A 按下");   // 刚按下
//     };
//     app.draw = [&] { Renderer::drawCircle(p.x, p.y, 16, Color::Yellow); };
//     return app.run();
// }
//
// 【玩法支撑】背包 / 交互 / 对话 / 任务 综合示例
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
// int main() {
//     App app; app.title = "玩法支撑";
//     Inventory bag(8); Quest quest; Dialog dialog; Font font;
//     std::vector<PickupInteractable> pickups;
//     app.setup = [&] {
//         registerItem({"carrot", "萝卜", "新鲜胡萝卜", 99, false, 5});
//         registerItem({"apple",  "苹果", "脆甜苹果",   99, false, 3});
//         PickupInteractable a; a.position = {200, 200}; a.itemId = "carrot"; a.inventory = &bag; pickups.push_back(a);
//         PickupInteractable b; b.position = {300, 300}; b.itemId = "apple";  b.inventory = &bag; pickups.push_back(b);
//         quest.id = "harvest"; quest.title = "收获季"; quest.objectiveId = "carrot"; quest.required = 3; quest.start();
//         DialogNode n0; n0.speaker = "NPC"; n0.text = "想听什么?"; n0.choices = {"讲个故事", "再见"}; n0.next = {1, -1};
//         DialogNode n1; n1.speaker = "NPC"; n1.text = "从前有座农场……";
//         dialog.addNode(n0); dialog.addNode(n1);
//         if (!font.load("font.ttf", 16)) Log::error("font.ttf 缺失, 不画文字");
//     };
//     app.update = [&](float) {
//         if (Input::isKeyPressed(GLFW_KEY_E))                       // 拾取 + 任务推进
//             for (auto& p : pickups)
//                 if (!p.picked && p.inRange(Vec2{400, 300}, Vec2{16, 16})) { p.onInteract(); quest.advance(1); }
//         if (Input::isKeyPressed(GLFW_KEY_T)) dialog.start();       // 开始对话
//         if (Input::isKeyPressed(GLFW_KEY_SPACE)) {                 // 推进/选择
//             if (dialog.current() && !dialog.current()->choices.empty()) dialog.choose(0);
//             else dialog.advance();
//         }
//     };
//     app.draw = [&] {
//         if (font.valid()) {
//             char buf[128];
//             snprintf(buf, sizeof(buf), "背包: 萝卜 %d | 任务: %d/%d %s",
//                      bag.count("carrot"), quest.progress, quest.required, quest.isCompleted() ? "[完成]" : "");
//             font.drawText(10, 10, buf, Color::White, 1.0f);
//             if (const DialogNode* n = dialog.current())
//                 font.drawText(10, 40, (n->speaker + ": " + n->text).c_str(), Color::Yellow, 1.0f);
//         }
//     };
//     return app.run();
// }
//
// ############################################################################
// 【验收示例】星露谷式农场游戏(约 320 行)。覆盖: 瓦片地图 / 角色移动 / 碰撞 /
//   交互拾取 / 种植 / 背包 / 对话 / 任务 / 存档 / 音效 / UI
// 操作: WASD 移动, E 交互(拾取/对话/收获), F 种种子, Q 卖萝卜(靠近爷爷),
//       I 开关背包, F5 存档, F9 读档
// ----------------------------------------------------------------------------
// #define LGNITE_IMPLEMENTATION
// #include "lgnite.h"
// using namespace lgnite;
//
// // ---------- 全局资源与状态 ----------
// Texture gTiles; Font gFont; AudioClip gPick, gQuest, gHarvest;
// Tilemap gMap; Camera2D gCam; Inventory gBag(24); Quest gQuestHarvest; Dialog gDialog;
// RigidBody2D gBody{{3 * 32.f, 3 * 32.f}, {20, 26}};   // 玩家刚体(左上角 + 尺寸)
// std::vector<PickupInteractable> gPickups;            // 地上的拾取物
// Vec2 gNpc{20 * 32.f, 6 * 32.f};                      // 爷爷 NPC 位置
// int gGold = 0; bool gShowBag = false; bool gInDialog = false;
// std::string gHint;                                    // 当前交互提示
//
// // 作物: 种在耕地, 随时间生长, 成熟后可收获
// struct Crop { int gx, gy; std::string itemId; float growTime = 0; float growDuration = 6.f; bool grown = false; };
// std::vector<Crop> gCrops;
//
// // ---------- 小工具: 物品 id -> 占位颜色(无贴图时) ----------
// Color itemColor(const std::string& id) {
//     if (id == "carrot") return {1.0f, 0.55f, 0.2f, 1};
//     if (id == "turnip") return {0.8f, 0.4f, 0.9f, 1};
//     if (id == "apple")  return {1.0f, 0.2f, 0.2f, 1};
//     if (id == "seed")   return {0.65f, 0.45f, 0.2f, 1};
//     return Color::White;
// }
//
// // ---------- 搭建地图: 草地 + 边缘水 + 障碍 + 耕地 ----------
// void buildMap() {
//     gMap.create(40, 30, 32);
//     if (gTiles.valid()) gMap.setTileset(gTiles, 8);
//     for (int x = 0; x < 40; ++x) for (int y = 0; y < 30; ++y) gMap.set(x, y, 0);   // 草地
//     for (int x = 0; x < 40; ++x) { gMap.set(x, 0, 2); gMap.set(x, 29, 2); }       // 上下水
//     for (int y = 0; y < 30; ++y) { gMap.set(0, y, 2); gMap.set(39, y, 2); }       // 左右水
//     const int rocks[][2] = {{6,5},{7,5},{15,12},{16,12},{25,8},{28,20},{10,22},{22,24}};  // 石头(3)
//     for (auto& r : rocks) gMap.set(r[0], r[1], 3);
//     const int trees[][2] = {{5,10},{9,14},{18,6},{30,14},{14,25},{33,22}};               // 树(4)
//     for (auto& t : trees) gMap.set(t[0], t[1], 4);
//     for (int x = 4; x <= 10; ++x) for (int y = 18; y <= 22; ++y) gMap.set(x, y, 1);     // 耕地(1)
//     gMap.setTileSolid(2, true); gMap.setTileSolid(3, true); gMap.setTileSolid(4, true);  // 水/石/树不可走
// }
//
// // ---------- 撒拾取物 ----------
// void spawnPickups() {
//     auto add = [&](const char* id, float x, float y) {
//         PickupInteractable p; p.itemId = id; p.position = {x, y}; p.size = {24, 24};
//         p.prompt = "按 E 拾取"; p.inventory = &gBag; gPickups.push_back(p);
//     };
//     add("carrot", 5 * 32.f + 4, 19 * 32.f + 4);
//     add("carrot", 7 * 32.f + 4, 20 * 32.f + 4);
//     add("carrot", 9 * 32.f + 4, 21 * 32.f + 4);
//     add("turnip", 12 * 32.f + 4, 10 * 32.f + 4);
//     add("apple",  26 * 32.f + 4, 6 * 32.f + 4);
//     add("seed",   30 * 32.f + 4, 18 * 32.f + 4);
// }
//
// // ---------- 对话树(带分支) ----------
// void buildDialog() {
//     DialogNode n0; n0.speaker = "爷爷"; n0.text = "欢迎来到农场! 帮我收 3 个萝卜吧。"; gDialog.addNode(n0);
//     DialogNode n1; n1.speaker = "爷爷"; n1.text = "你愿意帮忙吗?";
//     n1.choices = {"当然愿意!", "我先逛逛"}; n1.next = {2, 3}; gDialog.addNode(n1);
//     DialogNode n2; n2.speaker = "爷爷"; n2.text = "太好了! 去田里(棕色区)按 E 收萝卜。"; gDialog.addNode(n2);
//     DialogNode n3; n3.speaker = "爷爷"; n3.text = "好的, 随时回来。"; gDialog.addNode(n3);
// }
//
// // ---------- 存档 / 读档 ----------
// void saveGame() {
//     std::unordered_map<std::string, std::string> kv;
//     kv["px"] = std::to_string(gBody.position.x); kv["py"] = std::to_string(gBody.position.y);
//     kv["gold"] = std::to_string(gGold); kv["quest"] = std::to_string(gQuestHarvest.progress);
//     kv["bag"] = gBag.serialize();
//     SaveSystem::save("farm.sav", kv); Log::info("已存档");
// }
// void loadGame() {
//     std::unordered_map<std::string, std::string> kv;
//     if (!SaveSystem::load("farm.sav", kv)) { Log::warn("无存档"); return; }
//     gBody.position.x = SaveSystem::getFloat(kv, "px", gBody.position.x);
//     gBody.position.y = SaveSystem::getFloat(kv, "py", gBody.position.y);
//     gGold = SaveSystem::getInt(kv, "gold", 0);
//     gQuestHarvest.progress = SaveSystem::getInt(kv, "quest", 0);
//     if (gQuestHarvest.progress >= gQuestHarvest.required) gQuestHarvest.complete();
//     gBag.deserialize(SaveSystem::getStr(kv, "bag", "")); Log::info("已读档");
// }
//
// int main() {
//     App app; app.title = "星露谷式农场"; app.width = 960; app.height = 640;
//     app.setup = [&] {
//         if (!gTiles.load("tiles.png")) Log::error("tiles.png 缺失, 用色块占位");
//         if (!gFont.load("font.ttf", 16))  Log::error("font.ttf 缺失, 不画文字");
//         if (!gPick.load("pick.wav"))      Log::warn("pick.wav 缺失, 无拾取音效");
//         if (!gQuest.load("quest.wav"))    Log::warn("quest.wav 缺失, 无任务音效");
//         if (!gHarvest.load("harvest.wav"))Log::warn("harvest.wav 缺失, 无收获音效");
//         registerItem({"carrot", "萝卜", "新鲜胡萝卜", 99, false, 5});
//         registerItem({"turnip", "芜菁", "大芜菁",     99, false, 8});
//         registerItem({"apple",  "苹果", "脆甜苹果",   99, false, 3});
//         registerItem({"seed",   "种子", "作物种子",   99, false, 2});
//         buildMap(); spawnPickups(); buildDialog();
//         gQuestHarvest.id = "harvest"; gQuestHarvest.title = "收获季";
//         gQuestHarvest.objectiveId = "carrot"; gQuestHarvest.required = 3; gQuestHarvest.start();
//         gBody.gravity = 0;                                   // 俯视角, 无重力
//         gCam.clampEnabled = true; gCam.clampRect = {0, 0, 40 * 32.f, 30 * 32.f};
//         gCam.pos = gBody.position;
//         loadGame();
//     };
//     app.update = [&](float dt) {
//         // ---- 对话中: 空格/E 推进或选择 ----
//         if (gInDialog) {
//             if (Input::isKeyPressed(GLFW_KEY_SPACE) || Input::isKeyPressed(GLFW_KEY_E)) {
//                 if (gDialog.current() && !gDialog.current()->choices.empty()) gDialog.choose(0);
//                 else gDialog.advance();
//                 if (gDialog.finished()) gInDialog = false;
//             }
//             gCam.follow(gBody.position, dt, 960, 640);
//             return;
//         }
//         // ---- 移动 + 瓦片碰撞(俯视角, 无重力) ----
//         Vec2 dir{0, 0};
//         if (Input::isKeyDown(GLFW_KEY_W)) dir.y -= 1;
//         if (Input::isKeyDown(GLFW_KEY_S)) dir.y += 1;
//         if (Input::isKeyDown(GLFW_KEY_A)) dir.x -= 1;
//         if (Input::isKeyDown(GLFW_KEY_D)) dir.x += 1;
//         if (dir.x != 0 && dir.y != 0) dir = dir * 0.7071f;   // 对角归一化
//         gBody.velocity = dir * 160.f;
//         moveAndCollide(gBody, gMap, dt);                     // 一行解决移动+碰撞
//         // ---- 作物生长 ----
//         for (auto& c : gCrops) {
//             if (c.grown) continue;
//             c.growTime += dt;
//             if (c.growTime >= c.growDuration) c.grown = true;
//         }
//         // ---- 交互检测 ----
//         gHint.clear();
//         bool nearNpc = Vec2::dist(gBody.position, gNpc) < 48.f;
//         for (auto& p : gPickups) {
//             if (p.picked) continue;
//             if (p.inRange(gBody.position, gBody.size)) {
//                 gHint = p.prompt;
//                 if (Input::isKeyPressed(GLFW_KEY_E)) {
//                     p.onInteract();                          // 加入背包
//                     if (p.itemId == "carrot") { gQuestHarvest.advance(1); if (gQuestHarvest.isCompleted()) gQuest.play(); }
//                     else gPick.play();
//                 }
//             }
//         }
//         // 收获成熟作物
//         for (auto it = gCrops.begin(); it != gCrops.end();) {
//             Crop& c = *it;
//             Vec2 cp{c.gx * 32.f, c.gy * 32.f};
//             if (c.grown && Vec2::dist(gBody.position, cp) < 40.f) {
//                 gHint = "按 E 收获";
//                 if (Input::isKeyPressed(GLFW_KEY_E)) {
//                     const Item* item = findItem(c.itemId);
//                     if (item) { gBag.add(*item, 1); gHarvest.play(); }
//                     it = gCrops.erase(it); continue;
//                 }
//             }
//             ++it;
//         }
//         // 种植: 站在耕地按 F 消耗种子
//         if (Input::isKeyPressed(GLFW_KEY_F) && gBag.count("seed") > 0) {
//             int gx = (int)std::floor((gBody.position.x + gBody.size.x * 0.5f) / 32.f);
//             int gy = (int)std::floor((gBody.position.y + gBody.size.y * 0.5f) / 32.f);
//             if (gMap.get(gx, gy) == 1) { gBag.remove("seed", 1); gCrops.push_back({gx, gy, "carrot", 0, 6.f, false}); }
//         }
//         if (nearNpc) {
//             gHint = "按 E 与爷爷对话 (Q 卖萝卜)";
//             if (Input::isKeyPressed(GLFW_KEY_E)) { gDialog.start(); gInDialog = true; }
//             if (Input::isKeyPressed(GLFW_KEY_Q) && gBag.count("carrot") > 0) {
//                 gBag.remove("carrot", 1); gGold += 5; gQuest.play();
//             }
//         }
//         if (Input::isKeyPressed(GLFW_KEY_I)) gShowBag = !gShowBag;
//         if (Input::isKeyPressed(GLFW_KEY_F5)) saveGame();
//         if (Input::isKeyPressed(GLFW_KEY_F9)) loadGame();
//         gCam.follow(gBody.position, dt, 960, 640);
//     };
//     app.draw = [&] {
//         Renderer::setCamera(&gCam);
//         gMap.draw();
//         if (!gTiles.valid())   // 无贴图: 障碍画占位块
//             for (int y = 0; y < gMap.height(); ++y) for (int x = 0; x < gMap.width(); ++x)
//                 if (gMap.isTileSolid(x, y)) Renderer::drawRect(x * 32.f, y * 32.f, 32, 32, Color::Gray);
//         // 作物(成熟=大, 未熟=小)
//         for (auto& c : gCrops) {
//             float r = c.grown ? 10.f : 5.f;
//             Renderer::drawCircle(c.gx * 32.f + 16, c.gy * 32.f + 16, r, itemColor(c.itemId));
//         }
//         // 拾取物
//         for (auto& p : gPickups) if (!p.picked)
//             Renderer::drawCircle(p.position.x + 12, p.position.y + 12, 10, itemColor(p.itemId));
//         // NPC(爷爷)
//         Renderer::drawRect(gNpc.x, gNpc.y, 28, 36, Color{0.9f, 0.85f, 0.7f, 1});
//         if (gFont.valid()) gFont.drawText(gNpc.x, gNpc.y - 16, "爷爷", Color::White, 0.7f);
//         // 玩家
//         Renderer::drawRect(gBody.position.x, gBody.position.y, gBody.size.x, gBody.size.y, Color::Cyan);
//         Renderer::setCamera(nullptr);
//         // ---- UI(屏幕坐标) ----
//         if (gFont.valid()) {
//             char buf[160];
//             snprintf(buf, sizeof(buf), "任务: %s %d/%d    金币: %d",
//                      gQuestHarvest.title.c_str(), gQuestHarvest.progress, gQuestHarvest.required, gGold);
//             gFont.drawText(10, 10, buf, Color::White, 1.0f);
//             if (!gHint.empty()) gFont.drawText(10, 40, gHint.c_str(), Color::Yellow, 1.0f);
//             if (gInDialog && gDialog.current()) {
//                 const DialogNode* n = gDialog.current();
//                 std::string line = n->speaker + ": " + n->text;
//                 Renderer::drawRect(80, 440, 800, 80, Color{0, 0, 0, 0.7f});
//                 gFont.drawText(100, 460, line.c_str(), Color::White, 1.0f);
//                 if (!n->choices.empty()) gFont.drawText(100, 492, "空格/E: 选择第一项", Color::Green, 0.8f);
//             }
//         }
//         // 背包栏(底部)
//         if (gShowBag) {
//             Renderer::drawRect(100, 500, 760, 120, Color{0, 0, 0, 0.8f});
//             for (int i = 0; i < gBag.capacity(); ++i) {
//                 float bx = 110 + i * 32.f, by = 510;
//                 Renderer::drawRect(bx, by, 28, 28, Color{0.2f, 0.2f, 0.25f, 1});
//                 const ItemStack& s = gBag.slot(i);
//                 if (s.count > 0) {
//                     Renderer::drawRect(bx + 4, by + 4, 20, 20, itemColor(s.item.id));
//                     if (gFont.valid()) gFont.drawText(bx + 8, by + 6, std::to_string(s.count).c_str(), Color::White, 0.6f);
//                 }
//             }
//         }
//     };
//     return app.run();
// }
//
// 【Sprite -> ECS 迁移建议】
//   1) 先把 Actor 里的 pos/rotation/scale 搬成 Transform 组件, Sprite 直接作组件;
//   2) update() 里的移动逻辑搬进 System(用 world.each 批量处理);
//   3) ActorGroup 职责交给 World, Actor 体系即可整体退役, 渲染/输入/音频 API 不变。
//
// 【注意事项】
//   - LGNITE_IMPLEMENTATION 只能在【一个】翻译单元定义, 否则重复定义链接错误;
//   - 手动流程必须先 Window::create() 再 gl::init()(GL 函数依赖已创建的上下文);
//   - 单文件较大: 建议只在实现 .cpp 中定义实现宏, 其余文件包含开销很小;
//   - 需要新的 GL 函数时, 在实现区 LGNITE_GL_LIST 登记一行并重新编译;
//   - Font 默认仅烘焙 ASCII, 中文渲染需自行扩展烘焙范围(见实现区 Font 注释)。

#endif // LGNITE_H
