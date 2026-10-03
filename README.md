# WebSocket Gomoku · 在线五子棋

![在线五子棋主题图（概念配图，非运行截图）](docs/images/hero.png)

一个使用 **C++11、WebSocket++ 和 MySQL C API** 编写的浏览器五子棋学习项目。HTTP 处理注册、登录与用户资料，WebSocket 连接游戏大厅和双人房间，服务端负责匹配、棋盘状态、胜负判定与积分更新。

项目实现了从注册到一局对战结束的基础业务链路，适合学习实时通信、会话管理和模块拆分。当前代码存在已知安全与规则校验缺口，只适合使用测试账号在受控环境演示。

## 已实现的功能

- 浏览器注册、登录，展示积分、比赛场次与获胜场次。
- 游戏大厅开始/停止匹配，按积分分为三个队列：低于 2000、2000–2999、3000 及以上。
- 双人房间与 15 × 15 棋盘，Canvas 绘制棋盘和棋子。
- 横、竖、两个斜方向的五子连珠判定；当前规则为连续不少于五子。
- 房间内聊天、断线判负、结束后返回大厅。
- 新用户初始 1000 分；胜者加 30 分，败者减 30 分，同时更新对局统计。
- 内存会话、在线连接映射、匹配队列和房间生命周期管理。

以上是源码中可确认的能力；不代表已通过当前环境的构建、双客户端联调或压力测试。

## 架构与目录

![浏览器、服务端模块与数据库之间的关系](docs/images/architecture.svg)

| 路径 | 职责 |
| --- | --- |
| `source/gobang.cc` | 程序入口、数据库连接参数、监听端口 |
| `source/server.hpp` | HTTP 与 WebSocket 路由、连接与消息回调 |
| `source/db.hpp`、`source/db.sql` | 用户数据访问与建表脚本 |
| `source/session.hpp` | 会话与定时过期管理 |
| `source/online.hpp` | 大厅/房间用户 ID 与连接的映射 |
| `source/matcher.hpp` | 积分分档、匹配队列与工作线程 |
| `source/room.hpp` | 棋盘、落子、胜负、聊天广播与房间管理 |
| `source/wwwroot/` | 登录、注册、大厅、房间网页及静态资源 |
| `Online_backgammon/` | MySQL、JSON 和 WebSocket 的独立学习实验 |
| `WebBench-master/` | 仓库附带的 HTTP 压测工具 |

附带 WebBench 不等于已有性能结论；仓库没有提供可复核的五子棋服务压测结果。

## 通信流程

1. 浏览器通过 `POST /reg` 注册，通过 `POST /login` 登录；服务端返回 `SSID` Cookie。
2. 大厅通过 `GET /info` 获取资料，随后连接 `ws://<主机>:8085/hall`。
3. 玩家发送 `match_start` 或 `match_stop`；匹配成功后两人收到 `match_success`。
4. 房间页面连接 `/room`，接收房间号、玩家 ID 与黑白棋身份。
5. 客户端发送 `put_chess` 或 `chat` 消息；服务端更新房间并向双方广播。
6. 连珠或退出触发胜负处理，服务端更新数据库；用户返回大厅开始下一局。

前端目前让白棋先走。服务端还未完整校验轮次，因此该行为依赖网页客户端，见下文限制。

## 构建与运行

### 1. 准备依赖

现有服务端面向 Linux 风格的编译环境，需要：

- 支持 C++11 的 `g++`，以及 `make`（使用原构建文件时）。
- WebSocket++ 头文件。
- Boost.Asio / Boost.System 与 POSIX 线程支持。
- JsonCpp 开发头文件与链接库。
- MySQL 兼容 C API 开发头文件和 `libmysqlclient`。
- 可连接的、支持 SQL `PASSWORD()` 的数据库服务，以及现代浏览器。

**数据库兼容性是启动前提。** 现有注册和登录 SQL 使用 `PASSWORD()`。该函数已从 MySQL 8.0 移除，直接换成 MySQL 8.0 不能保持现有认证 SQL 可用。[MySQL 官方说明](https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html)

MariaDB 文档仍提供该函数，但本项目未给出经联调的数据库版本矩阵，不能据此保证某个版本完整兼容。[MariaDB 函数说明](https://mariadb.com/docs/server/reference/sql-functions/secondary-functions/encryption-hashing-and-compression-functions/password)

### 2. 获取项目并初始化独立测试数据库

```bash
git clone https://github.com/759stronger/websocket-gomoku.git
cd websocket-gomoku/source
```

**`db.sql` 第一行会执行 `DROP DATABASE IF EXISTS gobang`。只在不含业务数据的独立测试实例中初始化；不要在共享或正式数据库上直接执行。**

核对脚本后，使用有初始化权限的测试数据库账号执行；将 `YOUR_DB_ADMIN` 换成自己的账号，`-p` 会交互输入口令：

```bash
mysql -h 127.0.0.1 -u YOUR_DB_ADMIN -p < db.sql
```

为应用准备仅访问测试 `gobang` 数据库的专用账号。应用业务需要用户表的查询、插入和更新权限，不必沿用源码中的管理员账号。

### 3. 设置本地连接参数

连接参数位于 `source/gobang.cc` 顶部。按本地测试环境替换，下面只展示占位值：

```cpp
#define HOST "127.0.0.1"
#define USER "YOUR_DEMO_DB_USER"
#define PASS "YOUR_LOCAL_DB_PASSWORD"
#define DBNAME "gobang"
#define PORT 3306
```

仓库原有配置包含硬编码口令。本 README 不重复该口令；若原口令曾真实使用，应轮换。不要把自己替换后的真实凭据提交到公共仓库。

### 4. 编译并从静态资源所在目录启动

原 `source/makefile` 使用 `/usr/lib64/mysql` 作为库路径。依赖安装路径匹配时可执行 `make`。也可仅编译入口源文件，避免将头文件作为独立输入：

```bash
g++ -g -std=c++11 gobang.cc -o gobang \
  -lmysqlclient -ljsoncpp -lboost_system -pthread
./gobang
```

该命令要求依赖头文件和库已在编译器默认搜索路径；非标准安装位置需按实际环境补充 `-I` 和 `-L`。服务默认端口为 `8085`，静态资源根目录为相对路径 `./wwwroot/`，因此请在 `source/` 目录启动。

仓库附带了历史编译产物；演示时应从源码重新构建，避免依赖其平台和动态库环境。

### 5. 演示一局对战

1. 打开 `http://127.0.0.1:8085/register.html`，注册两个测试账号。
2. 使用两个独立浏览器会话登录 `http://127.0.0.1:8085/login.html`；可使用两个浏览器或普通窗口与隐私窗口，避免共享同一 Cookie。
3. 两个账号进入大厅并点击开始匹配；刚注册的账号会进入同一积分队列。
4. 匹配成功后交替落子，查看双方棋盘同步和聊天。
5. 对局结束后返回大厅，检查积分及对局统计。

如果资料接口失败，先核对数据库连接、`PASSWORD()` 兼容性和初始化结果；如果网页返回 404，核对启动目录和 `wwwroot/`；无法连接时核对端口占用和网络访问条件。

## 已知限制

- **登录结果处理有缺陷。** `server.hpp` 调用数据库登录函数后未保存其返回值，后续检查仍使用 JSON 解析结果；认证失败可能继续创建非预期会话。演示只使用成功注册的测试账号，不可将现有登录流程用于正式认证。
- **落子校验不完整。** 房间服务直接使用客户端传入的 UID、行列值，尚未完整绑定会话身份、检查坐标范围、约束轮次或拒绝已结束棋局的后续操作。自定义请求可能绕过前端规则，异常坐标存在越界访问风险。
- **数据库查询直接拼接输入。** 注册和登录通过 `sprintf` 拼接 SQL，未使用参数化查询且缺少输入长度约束，存在 SQL 注入及固定缓冲区风险；`PASSWORD()` 也不是面向本应用设计的现代密码存储方案。
- **浏览器输出和静态路径处理需加固。** 聊天内容使用 `innerHTML` 渲染，静态文件路径由请求 URI 拼接；尚需补齐输出转义和资源目录边界检查。
- 当前使用非 TLS WebSocket，未实现重连恢复、棋谱持久化、观战、满盘和棋或专业禁手规则。
- 未建立自动测试、持续集成和可靠的并发性能基准；学习用模块接口仍需异常路径和并发安全验证。

## 说明范围

本说明依据提交 `a808a2e6e4cbc54890f4c4137476cf3b83977959` 的目录、构建文件和业务代码整理，未在此次整理中运行仓库代码。主题图为概念配图，架构图为模块关系示意。

仓库尚未声明统一的项目许可证。附带工具及第三方库可能各有许可证，公开源码不代表可自动按任意开源许可证再发布。
