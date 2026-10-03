#ifndef __M_ROOM_H__
#define __M_ROOM_H__

#include "util.hpp"
#include "logger.hpp"
#include "db.hpp"
#include "online.hpp"

typedef enum
{
    GAME_START,
    GAME_OVER
} room_statu;

#define BOARD_ROW 15
#define BOARD_COL 15

#define CHESS_WHITE 1
#define CHESS_BLACK 2

class room
{
private:
    uint64_t _room_id;                    // 房间id  统一分配
    room_statu _statu;                    // 房间状态  直接初始化
    int _player_count;                    // 玩家数量  初始化为0
    uint64_t _white_id;                   // 白棋玩家id  添加的时候再设置
    uint64_t _black_id;                   // 黑棋玩家id
    user_table *_tb_user;                 // 用户信息表的句柄 数据库的
    online_manager *_online_user;         // 在线用户的管理句柄  通信连接的
    std::vector<std::vector<int>> _board; // 棋盘

private:
    bool five(int row, int col, int row_off, int col_off, int color)
    {
        // row和col是下棋位置 off是偏移量也是方向
        int count = 1;
        int search_row = row + row_off;
        int search_col = col + col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;               // 同色棋子就++
            search_row += row_off; // 检索相同方向的 直到棋子颜色不一样
            search_col += col_off;
        }
        search_row = row - row_off;
        search_col = col - col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;               // 同色棋子就++
            search_row -= row_off; // 检索相同方向的 直到棋子颜色不一样
            search_col -= col_off;
        }
        return (count >= 5);
    }

    uint64_t check_win(int row, int col, int color)
    {
        // 横行五星 ：统计下棋位置左右位置同色棋子数量 行不变 列变化
        if (five(row, col, 0, 1, color) ||
            // 纵列五星：列不变 行变
            five(row, col, 1, 0, color) ||
            // 正斜五星：右上与左下  行--  列++   行++ 列--
            five(row, col, -1, 1, color) ||
            // 反斜五星：左上 行-- 列--  右下 行++ 列++
            five(row, col, -1, -1, color)) // 任意一个方向出现五星
        {
            return color == CHESS_WHITE ? _white_id : _black_id;
        }

        return 0; // 返回胜利玩家id  没有返回0
    }

public:
    room(uint64_t room_id, user_table *tb_user, online_manager *online_user) :                    // 考虑什么成员初始化 什么成员需要外部传入
                                                                               _room_id(room_id), // 传入
                                                                               _statu(GAME_START),
                                                                               _player_count(0),
                                                                               _tb_user(tb_user),         // 传入
                                                                               _online_user(online_user), // 传入
                                                                               _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))
    {
        DLOG("%lu 房间创建成功", _room_id);
    }
    ~room()
    {
        DLOG("%lu 房间销毁成功", _room_id);
    }

    // 外界获取房间id
    uint64_t id()
    {
        return _room_id;
    }
    // 查看房间状态
    room_statu statu()
    {
        return _statu;
    }
    // 获取房间中用户数量
    int player_count()
    {
        return _player_count;
    }
    // 用户id的设置与获取  添加接口收到玩家的进入房间的长连接请求 建立成功就把它添加到房间里
    void add_white_user(uint64_t uid)
    {
        _white_id = uid;
        _player_count++;
    }
    void add_black_user(uint64_t uid)
    {
        _black_id = uid;
        _player_count++;
    }
    uint64_t get_white_user() { return _white_id; }
    uint64_t get_black_user() { return _black_id; }

    // 下棋 聊天肯定都包含一个请求信息
    Json::Value handle_chess(Json::Value &req) // 处理下棋动作
    {
        Json::Value json_resp = req; // 任意一步不对返回响应结果

        // 1.当前请求的房间号是否与当前房间id匹配 外部已经做了 就不需要了

        // 2.判断两个玩家是否在线 一个不在线就是另一个胜利
        uint64_t cur_uid = req["uid"].asUInt64();
        int chess_row = req["row"].asInt();
        int chess_col = req["col"].asInt();
        if (_online_user->is_in_gameroom(_white_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线 不战而胜";
            json_resp["winner"] = (Json::UInt64)_black_id; // 注意类型的转化

            return json_resp;
        }
        if (_online_user->is_in_gameroom(_black_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线 不战而胜";
            json_resp["winner"] = (Json::UInt64)_white_id;

            return json_resp;
        }

        // 3.获取走棋的位置 判断是否合理
        if (_board[chess_row][chess_col] != 0)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "当前位置已经有棋";
            return json_resp;
        }
        // 走棋    走棋应该还要判断棋盘是否满了
        int cur_color = cur_uid == _white_id ? CHESS_WHITE : CHESS_BLACK;
        _board[chess_row][chess_col] = cur_color;

        // 4.走棋后判断是否胜利(当前位置)
        uint64_t winner_id = check_win(chess_row, chess_col, cur_color);
        if (winner_id != 0)
        {
            // 更新数据库信息 外部做了
            json_resp["reason"] = "五子连珠 胜利";
        }

        json_resp["result"] = true;
        json_resp["winner"] = (Json::UInt64)winner_id;

        return json_resp;
    }

    Json::Value handle_chat(Json::Value &req) // 处理聊天动作
    {
        Json::Value json_resp = req;
        // 检测房间号是否一致

        // 检测是否有敏感词  应该单独一个函数
        std::string msg = req["message"].asString();
        size_t pos = msg.find("垃圾");
        if (pos != std::string::npos)
        {

            json_resp["result"] = false;
            json_resp["reason"] = "消息不合规";
            return json_resp;
        }

        // 广播信息
        json_resp["result"] = true;
        return json_resp;
    }

    // 退出房间的处理   这个是连接断开后的处理 不是请求
    void handle_exit(uint64_t uid)
    {
        Json::Value json_resp;
        // 下棋中退出 对方胜利
        if (_statu == GAME_START)
        {
            uint64_t winner_id =(Json::UInt64)(uid == _white_id ? _black_id : _white_id);
            json_resp["optype"] = "put_chess";
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线";
            json_resp["room_id"] = (Json::UInt64)_room_id;
            json_resp["uid"] = (Json::UInt64)uid;
            json_resp["row"] = -1;
            json_resp["col"] = -1;
            json_resp["winner"] = (Json::UInt64)winner_id;

            uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
            _tb_user->win(winner_id);
            _tb_user->lose(loser_id);
            _statu = GAME_OVER;
            broadcast(json_resp);
        }

        // 下棋结束退出 正常结束
        _player_count--;
        return;
    }

    // 总的请求处理函数，函数内部区分请求类型，根据不同的请求调用不同的处理函数，得到响应进行广播
    void handle_request(Json::Value &req)
    {
        // 1.检验房间号
        Json::Value json_resp = req; // 任意一步不对返回响应结果

        uint64_t room_id = req["room_id"].asUInt64();
        if (room_id != _room_id)
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "房间号不匹配";
            return broadcast(json_resp);
        }

        // 2.根据请求类型调用函数
        if (req["optype"].asString() == "put_chess")
        {
            json_resp = handle_chess(req);
            if (json_resp["winner"].asUInt64() != 0) // 更新数据库
            {
                uint64_t winner_id = json_resp["winner"].asUInt64();
                uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                _tb_user->win(winner_id);
                _tb_user->lose(loser_id);
                _statu = GAME_OVER;
            }
        }
        else if (req["optype"].asString() == "chat")
        {
            json_resp = handle_chat(req);
        }
        else
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "未知请求类型";
        }

        std::string body;
        json_util::serialize(json_resp, body);
        DLOG("房间-广播：%s", body.c_str());
        return broadcast(json_resp);
    }

    // 将指定的信息广播给所有用户
    void broadcast(Json::Value &rsp) // 如果有观战 使用链表存储id循环发送
    {
        // 1.对要响应的信息json：：value中的数据进行序列化成一个字符串
        std::string body;
        json_util::serialize(rsp, body);

        // 2.获取房间中所有用户的通信连接  发送响应信息
        wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
        if (wconn.get() != nullptr)
        {
            wconn->send(body);
        }
        else
        {
            DLOG("游戏房间中白棋玩家连接获取失败");
        }
        wconn = _online_user->get_conn_from_room(_black_id);
        if (wconn.get() != nullptr)
        {
            wconn->send(body);
        }
        else
        {
            DLOG("游戏房间中黑棋玩家连接获取失败");
        }
        return;
    }
};

// 房间的管理模块
using room_ptr = std::shared_ptr<room>;

class room_manager
{
private:                // 管理的信息
    uint64_t _next_rid; // 房间id
    std::mutex _mutex;
    user_table *_tb_user;         // 信息表
    online_manager *_online_user; // 通信连接
    std::unordered_map<uint64_t, room_ptr> _rooms;
    std::unordered_map<uint64_t, uint64_t> _users;

public:
    // 初始化房间id计数器
    room_manager(user_table *ut, online_manager *om) : _next_rid(1), _tb_user(ut), _online_user(om)
    {
        DLOG("初始化成功");
    }
    ~room_manager()
    {
        DLOG("房间即将销毁");
    }

    // 创建房间  返回房间的智能指针管理对象
    room_ptr create_room(uint64_t uid1, uint64_t uid2)
    {
        // 1.校验是否都在大厅 都在才创建房间
        if (_online_user->is_in_gamehall(uid1) == false)
        {
            DLOG("用户不在大厅 %lu  创建房间失败", uid1);
            return room_ptr();
        }
        if (_online_user->is_in_gamehall(uid2) == false)
        {
            DLOG("用户不在大厅 %lu  创建房间失败", uid2);
            return room_ptr();
        }

        // 2.创建房间 将用户信息添加到房间中
        std::unique_lock<std::mutex> lock(_mutex); // 自动加锁
        room_ptr rp(new room(_next_rid, _tb_user, _online_user));
        rp->add_black_user(uid1);
        rp->add_white_user(uid2);

        // 3.将房间信息管理起来
        _rooms.insert(std::make_pair(_next_rid, rp));
        _users.insert(std::make_pair(uid1, _next_rid));
        _users.insert(std::make_pair(uid2, _next_rid));
        _next_rid++;
        // 4.返回房间信息
        return rp;
    }

    // 查找房间  通过房间id或用户id获取房间信息
    room_ptr get_room_by_rid(uint64_t rid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _rooms.find(rid);
        if (it == _rooms.end())
        {
            return room_ptr(); // 没找到 返回空的智能指针
        }
        return it->second; // 存放的就是智能指针
    }

    room_ptr get_room_by_uid(uint64_t uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto uit = _users.find(uid); // 用户id找房间id
        if (uit == _users.end())
        {
            return room_ptr();
        }
        uint64_t rid = uit->second;
        auto rit = _rooms.find(rid); // 房间id找智能指针  不能直接调用上面的函数 因为锁冲突 造成死锁
        if (rit == _rooms.end())
        {
            return room_ptr(); // 没找到 返回空的智能指针
        }
        return rit->second; // 存放的就是智能指针
    }

    // 销毁房间  通过房间id销毁
    void remove_room(uint64_t rid)
    {
        // 因为房间信息是通过shared——ptr再_rooms中进行管理，所以只要将shared_ptr从_rooms中移除
        // 则shared_ptr计数器为0，外界没有对房间信息进行操作保存的情况下就会释放

        // 关联的用户怎么办  用户信息没有移除

        // 通过房间id  获取房间信息
        room_ptr rp = get_room_by_rid(rid);
        if (rp.get() == nullptr)
        {
            return;
        }

        // 通过房间信息 获取用户id
        uint64_t uid1 = rp->get_white_user();
        uint64_t uid2 = rp->get_black_user();
        // 移除房间中用户信息
        std::unique_lock<std::mutex> lock(_mutex);
        _users.erase(uid1);
        _users.erase(uid2);

        // 移除房间管理信息
        _rooms.erase(rid);
    }

    // 删除房间中的用户，如果没有用户，就销毁房间 用户连接断开时调用
    void remove_room_user(uint64_t uid)
    {
        room_ptr rp = get_room_by_uid(uid); // 有用户要退出  获取房间信息
        if (rp.get() == nullptr)
        {
            return;
        }

        rp->handle_exit(uid);        // 处理房间中玩家退出动作
        if (rp->player_count() == 0) // 没有玩家 销毁房间
        {
            remove_room(rp->id());
        }
        return;
    }
};

#endif
