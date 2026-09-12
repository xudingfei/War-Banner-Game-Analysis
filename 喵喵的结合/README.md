# 喵喵的结合：随机地图机制


- resources.gpak 索引含 19,979 项，全部数据偏移加总与文件长度吻合。没有解包音频和图片。
- levels/ 下有 2,361 个 .lvl 文件，完整文件哈希去重为 2,344。包含测试和特殊关卡，不能把总数称为普通关卡数量。
- 所有 .lvl 开头的第二、第三个 32 位整数均为 10、10；将其解释为尺寸是对二进制格式的推断，不能据此说所有实际战场可通行区域都恰好是 10×10。完整记录结构尚未解码，初步通用记录解析假设未覆盖全部文件。
- levels/alley/easy/ 有 109 个关卡文件。data/maps/alley.gon 指定 folder alley，并将 easy、hard 都映射到 easy 目录，同时另列 rare、boss、miniboss、special 类别。这是本机版本的具体配置。
- data/tiles.gon 的 Grass 类型包含 GrassTile 80、TallGrassTile 15、BlankTile 5，Tall Grass 类型则是 15、80、5。这提供了按权重将模板格展开为实际地形的配置证据；实际运行频率以及是否还受其他条件影响尚未实测。
- data/spawns.gon 包含 PlayerSpawn、具体敌人和场景物件的编辑器编号。结合关卡文件与章节目录，可以判断存在预制战斗关卡与配置化生成机制；具体抽图概率、去重策略、镜像旋转规则和种子调用链仍不明确。
- data/maps/standard_nodes.gon 定义 battle、event、shop、boss 等节点类别；这些不能单独证明世界路线图的拓扑如何生成。路线图与单场棋盘应分开研究。
- 本机有汉化后的资源包及备份。抽查 alley.gon、tiles.gon、alley/easy/101.lvl 与现有备份字节一致；未声称整个资源包一致。
