#pragma once
#include <map>
class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)(); //��Ա����ָ��
};

