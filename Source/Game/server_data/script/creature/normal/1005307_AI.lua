--怪物喊话脚本


--怪物死亡喊话
function c1005307_OnDie(MapID, InstanceID, TargetID, TargetTypeID, RoleID)
       local temp = math.random(1,100)
	if temp>=1 and temp<=15 then
		--15%几率喊话
             cre.MonsterSay(MapID, InstanceID, TargetID, 20031)
       end	

end

aux.RegisterCreatureEvent(1005307, 4, "c1005307_OnDie")
