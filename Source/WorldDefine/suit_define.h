//--------------------------------------------------------------------------------------------
// Copyright (c) 2004 TENGWU Entertainment All rights reserved.
// filename: suit_define.h
// author: 
// actor:
// data: 2008-12-1
// last:
// brief: ��װ��ض���
//--------------------------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------------------------
// ����
//--------------------------------------------------------------------------------------------
const INT MAX_SUIT_ATT_NUM			= 5;	// ��װ(����)���Ը���
const INT MAX_SUIT_EQUIP_NUM		= 12;	// ��װ��ಿ������
const INT MIN_SUIT_EQUIP_NUM		= 2;	// ��װ���ٲ�������

//--------------------------------------------------------------------------------------------
// ��̬�ṹ
//--------------------------------------------------------------------------------------------
struct tagSuitProto
{
	DWORD	dwID;								// ��װ���
	DWORD	dwTriggerID[MAX_SUIT_ATT_NUM];		// Trigger ID
	DWORD	dwBuffID[MAX_SUIT_ATT_NUM];			// Buff ID
	INT8	n8ActiveNum[MAX_SUIT_ATT_NUM];		// �������Լ������
	INT8	n8SpecEffectNum;					// ��Ч�������
	INT8	n8Dummy[2];
};

//--------------------------------------------------------------------------------------------
// ���͵��ͻ�����Ϣ�ڵ����ݽṹ
//--------------------------------------------------------------------------------------------
struct tagSuitInit
{
	DWORD	dwSuitID;
	INT		nEquipNum;
};


//--------------------------------------------------------------------------------------------
// �ͻ��˾�̬�ṹ
//--------------------------------------------------------------------------------------------
struct tagSuitProtoClient : public tagSuitProto
{
	TCHAR		szSuitName[X_SHORT_NAME];
	TCHAR		szSpecEffectMod[X_SHORT_NAME];
};