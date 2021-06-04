# [施工中] 2048.c | 作为课程设计的 2048

[English](README.md)

这是一个作为我所在小组的程序设计应用基础课程设计项目的，（试图）仅使用 C 完成的 <ruby><rb>小游戏</rb><rt>轮子</rt></ruby> 。

**⚠注意：该项目仍在开发阶段。不同平台的版本均夹杂了致死量平台专属私货。不保证主分支代码的稳定性与安全性。**

## 命令行版 ( `2048.c` 和 `2048ForVS.c` )

`2048.c` 是命令行版的主要工作文件，在终端环境下编译并运行即可完整体验该项目，在 Ubuntu 20.04 focal 上、使用 clang version 10 通过测试。 `2048ForVS.c` （如果有）是兼容 Visual C++ 2008 的产物，使用 Visual C++ 2008 Express 及以上版本应当能正常运行。

### 游玩说明：

使用 WASD 或方向键控制， `q` 退出， `u` 后退一步， `r` 重新开始。

### 画饼列表：

- [X] 基本游戏逻辑
- [X] 后退
- [X] 着色数字背景
- [ ] 调整数字大小（至所有数字均占据大小相等的正方形）
- [X] 存储游戏记录
  - [X] 展示游戏排名
- [X] 图形用户界面

## 图形界面版 

`GUI-Ver` 为图形界面版的工作目录。它使用魔改版的 Nuklear 与 GDI 绘制，在 Windows 10 上使用 MinGW-w64 与 Visual C++ 2010 Express 均通过测试。理论上仅支持 Windows，应当能较轻松地迁移至其它桌面环境。

### 编译说明

#### 对于 Visual C++ 2010 Express（可能适用于其它 Visual C++）

- 直接编译运行即可！

#### 对于 MinGW-w64

- 编译时需要添加参数： `-lm -luser32 -lgdi32`

## Credit

[2048.c by mevdschee](https://github.com/mevdschee/2048.c) 的算法思想以及使用 non-canonical terminal input 的实践

[Nuklear by Immediate Mode UI](https://github.com/immediate-mode-ui/Nuklear/)

每一个向我提出建议的人

## License

[反996许可证](LICENSE)