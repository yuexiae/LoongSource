-- 怪物进入战斗触发示警
function c1008101_OnEnterCombat(MapID, InstanceID, CreatureID)
    --得到怪物的当前目标
    local TargetID = cre.GetCreatureCurTargetID(MapID, InstanceID, CreatureID)
    --同步仇恨给范围为8个格子，高度为20的友方怪物
    g_SynCreatureEnmity(MapID, InstanceID, CreatureID, TargetID, 1, 10, 20)
end

aux.RegisterCreatureEvent(1008101, 2, "c1008101_OnEnterCombat")





