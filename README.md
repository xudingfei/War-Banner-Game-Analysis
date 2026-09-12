# 战棋游戏随机地图机制分析

日期：2026-09-13。检查本机 Steam 安装的 Into the Breach 和 Mewgenics。仅只读检查安装资源，没有运行汉化脚本、修改游戏或存档。本次是资源结构与脚本验证，没有进行原游戏多种子开局回放。

## 陷阵之志：已确认

- 安装目录有 376 个 .map 文件，声明尺寸全部为 8×8。这包括特殊地图，不能等同于 376 张普通对战地图。
- 地图记录固定坐标及 terrain、建筑属性，并带 tags 与 zones。例：maps/acid0.map 包含 generic、acid、acid_pool、pistons 标签及 pistons 候选区域。
- scripts/missions/missions.lua 的 GetMapTag() 对标签表调用 random_element；GetMap() 从 MapList 随机取项。GetMapTag 的存在不代表已经还原引擎内全部地图选择逻辑。
- Mission:AddDefended 在地图指定区域里用 random_removal 取位置，随后设置地形及创建单位；mission_tanks.lua 声明 satellite 标签并调用 AddDefended。
- 因此可确认预制布局、任务标签、指定位置随机布置的机制。地图旋转/镜像、最终抽图权重、随机数发生器与调用顺序未完整验证。

开发者 Justin Ma 在采访中明确说明：基本布局人工设计，森林、沙地及任务特有建筑位置采用程序变化，以降低令人无解的布局出现概率。
来源：https://www.gamedeveloper.com/design/devs-weigh-in-on-the-best-ways-to-use-but-not-abuse-procedural-generation

## 喵喵的结合：已确认与推断

- resources.gpak 索引含 19,979 项，全部数据偏移加总与文件长度吻合。没有解包音频和图片。
- levels/ 下有 2,361 个 .lvl 文件，完整文件哈希去重为 2,344。包含测试和特殊关卡，不能把总数称为普通关卡数量。
- 所有 .lvl 开头的第二、第三个 32 位整数均为 10、10；将其解释为尺寸是对二进制格式的推断，不能据此说所有实际战场可通行区域都恰好是 10×10。完整记录结构尚未解码，初步通用记录解析假设未覆盖全部文件。
- levels/alley/easy/ 有 109 个关卡文件。data/maps/alley.gon 指定 folder alley，并将 easy、hard 都映射到 easy 目录，同时另列 rare、boss、miniboss、special 类别。这是本机版本的具体配置。
- data/tiles.gon 的 Grass 类型包含 GrassTile 80、TallGrassTile 15、BlankTile 5，Tall Grass 类型则是 15、80、5。这提供了按权重将模板格展开为实际地形的配置证据；实际运行频率以及是否还受其他条件影响尚未实测。
- data/spawns.gon 包含 PlayerSpawn、具体敌人和场景物件的编辑器编号。结合关卡文件与章节目录，可以判断存在预制战斗关卡与配置化生成机制；具体抽图概率、去重策略、镜像旋转规则和种子调用链仍不明确。
- data/maps/standard_nodes.gon 定义 battle、event、shop、boss 等节点类别；这些不能单独证明世界路线图的拓扑如何生成。路线图与单场棋盘应分开研究。
- 本机有汉化后的资源包及备份。抽查 alley.gon、tiles.gon、alley/easy/101.lvl 与现有备份字节一致；未声称整个资源包一致。

## 测试交付

Scripts/AuditInstalledMaps.py 可重复运行，仅读取两个游戏目录，把结果写进仓库 docs/InstalledMapAudit.json。检查索引边界、配置字段、全部关卡文件头、陷阵之志全部地图尺寸、标签选择函数和三个备份样本。

这些测试验证资源证据，不能替代原游戏运行时的相同种子复现、不同种子频率采样、固定模板的多次展开或难度变化对照。当前没有依据宣称已经恢复两款游戏完整算法，也没有依据说它们使用我们的 BFS 障碍拒绝生成器。

## 对当前虚幻项目的意义

现有生成器从空白棋盘随机放置障碍，再用 BFS 检查连通性。更接近这两款作品的下一步是：原创布局模板库 → 地区/任务标签筛选 → 可变格和出生区按规则展开 → 战术可用性检查。保留 C++ ECS 计算及蓝图表现分离即可。本次只做调查，没有改动现有生成器。

## 运行检查

需要 Python 3 和本机已安装的两款游戏。脚本仅使用 Python 标准库。

```powershell
python Scripts/AuditInstalledMaps.py --steam-common "D:\Game\steamapps\common"
```

结果写入 `docs/InstalledMapAudit.json`。统计基于本次检查的安装版本，后续游戏更新可能改变数量与配置。本仓库只包含分析、检查工具和统计结果，不包含游戏资源包、地图原文件或可执行程序。
