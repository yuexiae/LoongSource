--���ﺰ���ű�

-- �������ս������
function c1010055_OnEnterCombat(MapID, InstanceID, CreatureID)
        local temp = math.random(1,100)
	if temp>=1 and temp<=15 then
		--15%���ʺ���
	      cre.MonsterSay(MapID, InstanceID, CreatureID, 50043)
	end
end

aux.RegisterCreatureEvent(1010055, 2, "c1010055_OnEnterCombat")

--������������
function c1010055_OnDie(MapID, InstanceID, TargetID, TargetTypeID, RoleID)

	Increase_shili(MapID, InstanceID, RoleID, 1, 4, "jinjieshili")


end

aux.RegisterCreatureEvent(1010055, 4, "c1010055_OnDie")
