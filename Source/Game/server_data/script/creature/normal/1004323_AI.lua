--���ﺰ���ű�

-- �������ս������
function c1004323_OnEnterCombat(MapID, InstanceID, CreatureID)
       local temp = math.random(1,100)
	if temp>=1 and temp<=15 then
		--15%���ʺ���
             cre.MonsterSay(MapID, InstanceID, CreatureID, 10045)
       end	
end

aux.RegisterCreatureEvent(1004323, 2, "c1004323_OnEnterCombat")
