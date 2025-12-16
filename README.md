# The-rainbow
这是一个基于 C++ 和 OpenGL 3.3 开发的实时程序化景观渲染项目。该项目不使用任何外部贴图，完全通过着色器算法生成逼真的彩虹、山脉、草地和动态天空。/This is a real-time procedural landscape rendering project developed based on C++ and OpenGL 3.3. The project does not use any external textures, instead generating realistic rainbows, mountains, grasslands, and dynamic skies entirely through shader programming.
# OpenGL Realistic Procedural Scene: Rainbow & Mountains

这是一个基于 **C++** 和 **OpenGL 3.3** 开发的实时程序化景观渲染项目。该项目不使用任何外部贴图，完全通过**着色器算法（Shader Programming）**生成逼真的彩虹、山脉、草地和动态天空。

## 🌟 核心特性

- **程序化山脉渲染**：利用多层 **FBM (Fractal Brownian Motion)** 噪声生成起伏的山峦，并结合垂直岩层纹理（Vertical Rock Layers）模拟真实的侵蚀效果。
- **物理光照模拟**：通过有限差分法实时计算法线，实现山体随太阳角度变化的漫反射光照。
- **动态积雪系统**：根据山体高度（Snowline）和坡度（Slope）自动分配积雪分布。
- **物理光谱彩虹**：基于波长到 RGB 的转换算法（Wavelength-to-RGB），模拟真实的双层彩虹（主虹与霓虹）以及亚历山大暗带（Alexander's Dark Band）。
- **程序化草地**：使用高频噪声模拟细碎的草地颗粒感，并结合距离雾（Distance Fog）增强空间深远感。
- **环境交互**：支持通过键盘动态调整雨量（雨雾浓度）以及随时间自动移动的日照角度。

## 📸 效果展示

*项目包含以下渲染细节：*
1. **深色大气层**：模拟暴风雨后深邃的天空底色。
2. **岩层细节**：山体表面具有水平方向的岩石纹理，消除垂直方向的模糊感。
3. **光晕效果**：太阳周围的散射光晕效果。

## 🛠️ 技术栈

- **语言**: C++
- **图形 API**: OpenGL 3.3 (Core Profile)
- **窗口管理**: GLFW
- **加载器**: GLAD
- **数学库**: GLM (OpenGL Mathematics)

## 🚀 快速开始

### 前置条件
确保你的开发环境已配置以下库：
- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- [GLM](https://github.com/g-truc/glm)

### 编译与运行
1. 克隆仓库：
   ```bash
   git clone [https://github.com/你的用户名/项目名.git](https://github.com/你的用户名/项目名.git)
