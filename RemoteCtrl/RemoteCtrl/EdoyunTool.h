#pragma once
class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize)//���ṩ���ֽ����飨BYTE* pData��ת����һ��ʮ�������ַ�������ͨ�� OutputDebugStringA ���������Ϣ
	{
		std::string strOut;//��ת���ַ���
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0))//ÿ16�ֽڼ�һ�����з�
				strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}
};

