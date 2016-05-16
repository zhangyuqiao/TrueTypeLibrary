#include "TTF.h"

static void *(__stdcall *g_pAlloc)(size_t size) = nullptr;
static void (__stdcall *g_pFree)(void *p) = nullptr;

extern"C" uint8_t __stdcall TTLibraryInit(void *(* const pAlloc)(size_t size),void(* const pFree)(void *p))
{
	if (pAlloc == nullptr || pFree == nullptr)
		return TTINVALIDARGUMENTS;
		g_pAlloc = pAlloc;
		g_pFree = pFree;
	return TTSUCCESS;
}
extern"C" uint8_t __stdcall TTLibraryFree()
{
	if (g_pAlloc == nullptr || g_pFree == nullptr)
		return TTNOTINITIALISED;
	g_pAlloc = nullptr;
	g_pFree = nullptr;
	return TTSUCCESS;
}

inline uint16_t NetWorkToHost(uint16_t bigendian)
{
	union
	{
		uint8_t byte[2];
		uint16_t value;
	}ByteOrderTransform;
	ByteOrderTransform.byte[0] = reinterpret_cast<uint8_t *>(&bigendian)[1];
	ByteOrderTransform.byte[1] = reinterpret_cast<uint8_t *>(&bigendian)[0];
	return ByteOrderTransform.value;
}
inline int16_t NetWorkToHost(int16_t bigendian)
{
	union
	{
		uint8_t byte[2];
		int16_t value;
	}ByteOrderTransform;
	ByteOrderTransform.byte[0] = reinterpret_cast<uint8_t *>(&bigendian)[1];
	ByteOrderTransform.byte[1] = reinterpret_cast<uint8_t *>(&bigendian)[0];
	return ByteOrderTransform.value;
}
inline uint32_t NetWorkToHost(uint32_t bigendian)
{
	union
	{
		uint8_t byte[4];
		uint32_t value;
	}ByteOrderTransform;
	ByteOrderTransform.byte[0] = reinterpret_cast<uint8_t *>(&bigendian)[3];
	ByteOrderTransform.byte[1] = reinterpret_cast<uint8_t *>(&bigendian)[2];
	ByteOrderTransform.byte[2] = reinterpret_cast<uint8_t *>(&bigendian)[1];
	ByteOrderTransform.byte[3] = reinterpret_cast<uint8_t *>(&bigendian)[0];
	return ByteOrderTransform.value;
}
inline int32_t NetWorkToHost(int32_t bigendian)
{
	union
	{
		uint8_t byte[4];
		int32_t value;
	}ByteOrderTransform;
	ByteOrderTransform.byte[0] = reinterpret_cast<uint8_t *>(&bigendian)[3];
	ByteOrderTransform.byte[1] = reinterpret_cast<uint8_t *>(&bigendian)[2];
	ByteOrderTransform.byte[2] = reinterpret_cast<uint8_t *>(&bigendian)[1];
	ByteOrderTransform.byte[3] = reinterpret_cast<uint8_t *>(&bigendian)[0];
	return ByteOrderTransform.value;
}
//注意字节序
#ifndef MakeFourCC
#define MakeFourCC(ch0,ch1,ch2,ch3) (static_cast<uint32_t>(ch0) | (static_cast<uint32_t>(ch1) << 8U) |  (static_cast<uint32_t>(ch2) << 16U) | (static_cast<uint32_t>(ch3) << 24U))
#endif

struct TTFont:TTHFont__
{
	void *pFontData;
	void *pFontDataEnd;
	void *pUCS2MapTable;
	void *pUCS4MapTable;
	void *pLocalTable;
	void *pGlyphTable;
	TTRect bound;
	bool bIsIndexUInt32;
};

extern"C" uint8_t __stdcall TTFontLoadFromMemory(const void *pData, size_t size, TTHFont *phFont)
{
	if (g_pAlloc == nullptr)
		return TTNOTINITIALISED;

	TTFont *pFont = static_cast<TTFont *>(g_pAlloc(sizeof(TTFont)));
	if (pFont == nullptr)
		return TTMEMORYALLOCFAILED;
	
	pFont->pFontData = g_pAlloc(size);
	if (pFont->pFontData == nullptr)
		return TTMEMORYALLOCFAILED;

	uint8_t *pDest = static_cast<uint8_t *>(pFont->pFontData);
	const uint8_t *pSrc = static_cast<const uint8_t *>(pData);
	for (size_t i = 0U; i < size; ++i)
		pDest[i] = pSrc[i];
	
	pFont->pFontDataEnd = static_cast<uint8_t *>(pFont->pFontData) + size;

	//字体目录首部
	//uint32_t scalertype 4
	//uint16_t numTables 2 字体目录中目录项的个数
	//uint16_t searchRange 2
	//uint16_t entrySelector 2
	//uint16_t rangeShift 2
	
	//访问字体目录首部的numTables成员
	uint16_t numTables;
	{
		uint16_t *pnumTables = reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(pFont->pFontData) + 4);
		if ((pnumTables + 1) > pFont->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		numTables = NetWorkToHost(*pnumTables);
	}
	
	//目录项
	//uint32_t tag 4
	//uint32_t checkSum 4
	//uint32_t offset 4
	//uint32_t length 4
	
	//访问字体目录中的第一个目录项的地址
	uint8_t *pDirectoryItemArray = static_cast<uint8_t *>(pFont->pFontData) + 12;
	
	//在下文中用于判断，如果有表未找到，说明TTF文件已被损坏
	bool bIsCampNotFound = true;
	bool bIsLocaNotFound = true;
	bool bIsGlyfNotFound = true;
	bool bIsHeadNotFound = true;

	for (int i = 0; i < numTables; ++i)
	{
		//访问第i个目录项
		uint8_t *pDirectoryItem = pDirectoryItemArray + 16 * i;
		//访问第i个目录项的tag成员
		uint32_t *ptag = reinterpret_cast<uint32_t *>(pDirectoryItem);
		if ((ptag + 1) > pFont->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		switch (*ptag)
		{
		case MakeFourCC('c', 'm', 'a', 'p'):
		{
			bIsCampNotFound = false;
			//访问第i个目录项的offset成员
			uint32_t offset;
			{
				uint32_t *poffset = reinterpret_cast<uint32_t *>(pDirectoryItem + 8);
				if ((poffset + 1) > pFont->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				offset = NetWorkToHost(*poffset);
			}
			//访问cmap表的首地址
			void *pcmap = static_cast<uint8_t *>(pFont->pFontData) + offset;
			
			//cmap表目录首部
			//uint16_t version 2
			//uint16_t numberSubtables 2
			
			//访问cmap表目录首部的numberSubtables成员
			uint16_t numberSubtables;
			{
				uint16_t *pnumberSubtables = reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(pcmap) + 2);
				if ((pnumberSubtables + 1) > pFont->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				numberSubtables = NetWorkToHost(*pnumberSubtables);
			}
			
			//cmap表目录项
			//uint16_t platformID 2
			//uint16_t platformSpecificID 2
			//uint32_t offset 4
			
			//访问cmap表目录项中的第一个目录项的地址
			void *pcampItemArray = static_cast<uint8_t *>(pcmap) + 4;

			//初始化为nullptr，在下文中用于判断，以优先使用Windows平台
			pFont->pUCS2MapTable = nullptr;
			pFont->pUCS4MapTable = nullptr;

			for (int i = 0; i < numberSubtables; ++i)
			{
				//访问第i个目录项
				uint8_t *pcampItem = static_cast<uint8_t *>(pcampItemArray) + 8 * i;
				//访问第i个目录项的platformSpecificID和platformID成员
				uint16_t platformID;
				{
					uint16_t *pplatformID = reinterpret_cast<uint16_t *>(pcampItem);
					if ((pplatformID + 1) > pFont->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					platformID = NetWorkToHost(*pplatformID);
				}
				uint16_t platformSpecificID;
				{
					uint16_t *pplatformSpecificID = reinterpret_cast<uint16_t *>(pcampItem + 2);
					if ((pplatformSpecificID + 1) > pFont->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					platformSpecificID = NetWorkToHost(*pplatformSpecificID);
				}
				//platformID为3表示Windows时，platformSpecificID为1表示UCS2，为10表示UCS4
				//platformID为0表示Unicode时，platformSpecificID为3表示UCS2，为4表示UCS4
				
				//访问第i个目录项的offset成员
				uint32_t offset;
				{
					uint32_t *poffset = reinterpret_cast<uint32_t *>(pcampItem + 4);
					if ((poffset + 1) > pFont->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offset = NetWorkToHost(*poffset);
				}

				if (platformID == 3)
				{
					if (platformSpecificID == 1)
					{
						pFont->pUCS2MapTable = static_cast<uint8_t *>(pcmap) + offset;
					}
					else if (platformSpecificID == 10)
					{
						pFont->pUCS4MapTable = static_cast<uint8_t *>(pcmap) + offset;
					}
				}
				else if (platformID == 0)
				{
					if (platformSpecificID == 3)
					{
						if(pFont->pUCS2MapTable == nullptr)//优先使用Windows平台
							pFont->pUCS2MapTable = static_cast<uint8_t *>(pcmap) + offset;
					}
					else if (platformSpecificID == 4)
					{
						if (pFont->pUCS4MapTable == nullptr)//优先使用Windows平台
							pFont->pUCS4MapTable = static_cast<uint8_t *>(pcmap) + offset;
					}
				}
			}
		}
		break;
		case MakeFourCC('l', 'o', 'c', 'a'):
		{
			bIsLocaNotFound = false;
			//访问第i个目录项的offset成员
			uint32_t offset;
			{
				uint32_t *poffset = reinterpret_cast<uint32_t *>(pDirectoryItem + 8);
				if ((poffset + 1) > pFont->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				offset = NetWorkToHost(*poffset);
			}
			//访问loca表的首地址
			pFont->pLocalTable = static_cast<uint8_t *>(pFont->pFontData) + offset;
		}
		break;
		case MakeFourCC('g', 'l', 'y', 'f'):
		{
			bIsGlyfNotFound = false;
			//访问第i个目录项的offset成员
			uint32_t offset;
			{
				uint32_t *poffset = reinterpret_cast<uint32_t *>(pDirectoryItem + 8);
				if ((poffset + 1) > pFont->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				offset = NetWorkToHost(*poffset);
			}
			//访问glyf表的首地址
			pFont->pGlyphTable = static_cast<uint8_t *>(pFont->pFontData) + offset;
		}
		break;
		case MakeFourCC('h', 'e', 'a', 'd'):
		{
			bIsHeadNotFound = false;
			//访问第i个目录项的offset成员
			uint32_t offset;
			{
				uint32_t *poffset = reinterpret_cast<uint32_t *>(pDirectoryItem + 8);
				if ((poffset + 1) > pFont->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				offset = NetWorkToHost(*poffset);
			}
			//访问heap表的首地址
			void *pHead = static_cast<uint8_t *>(pFont->pFontData) + offset;
			
			//head表
			//uint16_t[2] version; 4
			//uint16_t[2] fontRevision 4
			//uint32_t checkSumAdjustment; 4
			//uint32_t magicNumber 4
			//uint16_t flags 2
			//uint16_t unitsPerEm 2
			//uint32_t[2] created 8
			//uint32_t[2] modified 8
			//int16_t xMin 2
			//int16_t yMin 2
			//int16_t xMax 2
			//int16_t yMax 2
			//uint16_t macStyle 2
			//uint16_t lowestRecPPEM 2
			//int16_t fontDirectionHint 2
			//int16_t indexToLocFormat 2
			//int16_t glyphDataFormat 2
			
			//访问head表的xMin、yMin、xMax和yMax成员
			if ((static_cast<uint8_t *>(pHead) + 52) > pFont->pFontDataEnd)
				return TTINVALIEFONTFORMAT;

			pFont->bound.xMin = NetWorkToHost(*reinterpret_cast<int16_t *>(static_cast<uint8_t *>(pHead) + 36));
			pFont->bound.yMin = NetWorkToHost(*reinterpret_cast<int16_t *>(static_cast<uint8_t *>(pHead) + 38));
			pFont->bound.xMax = NetWorkToHost(*reinterpret_cast<int16_t *>(static_cast<uint8_t *>(pHead) + 40));
			pFont->bound.yMax = NetWorkToHost(*reinterpret_cast<int16_t *>(static_cast<uint8_t *>(pHead) + 42));
			
			//访问head表的indexToLocFormat成员
			//0代表uint16_t，1代表uint32_t
			pFont->bIsIndexUInt32 = NetWorkToHost(*reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(pHead) + 50)) != 0U;
		}
		break;
		}
	}
	if (bIsCampNotFound || bIsLocaNotFound | bIsGlyfNotFound | bIsHeadNotFound)
		return TTINVALIEFONTFORMAT;

	*phFont = pFont;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontFree(TTHFont hFont)
{
	if (g_pFree == nullptr)
		return TTNOTINITIALISED;
	if (hFont != nullptr)
	{
		if (static_cast<TTFont *>(hFont)->pFontData != nullptr)
			g_pFree(static_cast<TTFont *>(hFont)->pFontData);
		g_pFree(hFont);
	}
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontGetBound(TTHFont hFont, const TTRect **ppBound)
{
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	*ppBound = &static_cast<TTFont *>(hFont)->bound;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontIsUCS2Supported(TTHFont hFont, uint8_t *pbIsSupported)
{
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	*pbIsSupported = (static_cast<TTFont *>(hFont)->pUCS2MapTable != nullptr);
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontGetGlyphIDUCS2(TTHFont hFont, uint16_t UCS2CharCode, uint16_t *pGlyphID)
{
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	if (static_cast<TTFont *>(hFont)->pUCS2MapTable == nullptr)
		return TTINVALIDARGUMENTS;
	
	//Glyf表UCS2
	//uint16_t format 2
	//uint16_t length  2
	//uint16_t language 2
	//uint16_t segCountX2 2
	//uint16_t searchRange 2
	//uint16_t entrySelector 2
	//uint16_t rangeShift 2
	//uint16_t endCode[segCount] 2*(segCountX2/2)
	//uint16_t reservedPad 2
	//uint16_t startCode[segCount] 2*(segCountX2/2)
	//uint16_t idDelta[segCount] 2*(segCountX2/2)
	//uint16_t idRangeOffset[segCount] 2*(segCountX2/2)

	//访问Glyf表UCS2的segCountX2成员
	uint16_t segCount; 
	{
		uint16_t *psegCount = reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(static_cast<TTFont *>(hFont)->pUCS2MapTable) + 6);
		if ((psegCount + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		segCount = NetWorkToHost(*psegCount) / 2;
	}
	
	//访问Glyf表UCS2的endCode成员的首地址
	uint16_t *endCodeArray = reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(static_cast<TTFont *>(hFont)->pUCS2MapTable) + 14);
	//访问Glyf表UCS2的startCode成员的首地址
	uint16_t *startCodeArray = endCodeArray + segCount + 1;
	//访问Glyf表UCS2的idDelta成员的首地址
	uint16_t *idDeltaArray = startCodeArray + segCount;
	//访问Glyf表UCS2的idRangeOffset成员的首地址
	uint16_t *idRangeOffsetArray = idDeltaArray + segCount;

	//在逻辑上每个段（segment）都有endCode、startCode、idDelta和idRangeOffset四个成员

	//未找到则glyphID为0
	uint16_t glyphID = 0;
	for (uint16_t i = 0U; i < segCount; ++i)
	{
		uint16_t startCode; 
		{
			uint16_t *pstartCode = startCodeArray + i;
			if ((pstartCode + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			startCode = NetWorkToHost(*pstartCode);
		}
		uint16_t endCode;
		{ 
			uint16_t *pendCode = endCodeArray + i;
			if ((pendCode + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			endCode = NetWorkToHost(*pendCode);
		}
		if (startCode <= UCS2CharCode&&UCS2CharCode <= endCode)
		{
			uint16_t idRangeOffset;
			{
				uint16_t *pidRangeOffset = idRangeOffsetArray + i;
				if ((pidRangeOffset + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				idRangeOffset = NetWorkToHost(*pidRangeOffset);
			}
			uint16_t idDelta;
			{ 
				uint16_t *pidDelta = idDeltaArray + i;
				if ((pidDelta + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				idDelta = NetWorkToHost(*pidDelta);
			}

			//当idRangeOffset为0时
			if (idRangeOffset == 0)
			{
				//glyphID为UCS2CharCode + idDelta（以65536为模）
				glyphID = UCS2CharCode + idDelta;
			}
			//否则
			else
			{
				//glyphID的位置为段（Segment）的idRangeOffset成员的位置向后偏移idRangeOffset个字节再向后偏移（UCS2CharCode-startCode）*2个字节（这里我们技巧性地对uint16_t进行偏移，这样便不用*2）
				{
					uint16_t *pglyphID = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(idRangeOffsetArray + i) + idRangeOffset) + (UCS2CharCode - startCode);
					if ((pglyphID + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					glyphID = NetWorkToHost(*pglyphID);
				}
				//当glyphID不为0时
				if (glyphID != 0U)
					//glyphID还需要再加上idDelta（以65536为模）
					glyphID += idDelta;
			}
		}
	}
	*pGlyphID = glyphID;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontIsUCS4Supported(TTHFont hFont, uint8_t *pbIsSupported)
{
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	*pbIsSupported = (static_cast<TTFont *>(hFont)->pUCS4MapTable != nullptr);
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTFontGetGlyphIDUCS4(TTHFont hFont, uint32_t UCS4CharCode , uint16_t *pGlyphID)
{
	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;
	if (static_cast<TTFont *>(hFont)->pUCS4MapTable == nullptr)
		return TTINVALIDARGUMENTS;
	
	//Glyf表UCS4
	//uint16_t[2] format 4
	//uint32_t length 4
	//uint32_t language 4
	//uint32_t nGroups 4
	//struct{
	//uint32_t startCharCode 4
	//uint32_t endCharCode 4
	//uint32_t startGlyphCode 4
	//}group[nGroups] (4+4+4)*nGroups

	//访问Glyf表UCS4的nGroups成员
	uint32_t nGroups;
	{
		uint32_t *pnGroups = reinterpret_cast<uint32_t *>(static_cast<TTFont *>(hFont)->pUCS4MapTable) + 3;
		if ((pnGroups + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		nGroups = NetWorkToHost(*pnGroups);
	}
	//访问Glyf表UCS4的group成员
	void *pGroupArray = reinterpret_cast<uint8_t *>(static_cast<TTFont *>(hFont)->pUCS4MapTable) + 16;
	
	//未找到则glyphID为0
	uint16_t glyphID = 0;
	for (uint32_t i = 0U; i < nGroups; ++i)
	{
		void *pGroup = static_cast<uint8_t *>(pGroupArray) + 12 * i;
		uint32_t startCharCode;
		{
			uint32_t *pstartCharCode = reinterpret_cast<uint32_t *>(pGroup);
			if ((pstartCharCode + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			startCharCode = NetWorkToHost(*pstartCharCode);
		}
		uint32_t endCharCode;
		{ 
			uint32_t *pendCharCode = reinterpret_cast<uint32_t *>(pGroup) + 1;
			if ((pendCharCode + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			endCharCode = NetWorkToHost(*pendCharCode);
		}
		uint32_t startGlyphCode;
		{
			uint32_t *pstartGlyphCode = reinterpret_cast<uint32_t *>(pGroup) + 2;
			if ((pstartGlyphCode + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			startGlyphCode = NetWorkToHost(*pstartGlyphCode);
		}
		if (startCharCode <= UCS4CharCode&&UCS4CharCode <= endCharCode)
		{
			glyphID = startGlyphCode + (UCS4CharCode - startCharCode);
			break;
		}
	}
	*pGlyphID = glyphID;
	return TTSUCCESS;
}

struct TTGlyph:TTHGlyph__
{
	uint8_t bIsSimple;
	TTRect bound;
};

struct TTGlyphSimple :TTGlyph
{
	uint16_t contourNumber;
	uint16_t *pContourEndPointerIndex;
	uint16_t pointNumber;
	TTGlyphSimplePoint *pPoint;
};

struct TTGlyphCompound :TTGlyph
{
	uint16_t childNumber;
	TTGlyphCompoundChild *pChild;
};

extern"C" uint8_t __stdcall TTGlyphInit(TTHFont hFont, uint16_t glyphID, TTHGlyph *phGlyph)
{
	if (g_pAlloc == nullptr)
		return TTNOTINITIALISED;

	if (hFont == nullptr)
		return TTINVALIDARGUMENTS;

	//用Loca表找到Glyph在Glyf表中的位置
	uint8_t *pGlyphData;
	if (static_cast<TTFont *>(hFont)->bIsIndexUInt32)
		pGlyphData = reinterpret_cast<uint8_t *>(static_cast<uint8_t *>(static_cast<TTFont *>(hFont)->pGlyphTable) + NetWorkToHost(reinterpret_cast<uint32_t *>(static_cast<TTFont *>(hFont)->pLocalTable)[glyphID]));
	else
		//偏移为loca表中的值*2个字节（这里我们技巧性地对uint16_t进行偏移，这样便不用*2）
		pGlyphData = reinterpret_cast<uint8_t *>(static_cast<uint16_t *>(static_cast<TTFont *>(hFont)->pGlyphTable) + NetWorkToHost(reinterpret_cast<uint16_t *>(static_cast<TTFont *>(hFont)->pLocalTable)[glyphID]));

	//Glyph首部
	//int16_t numberOfContours 2
	//int16_t xMin 2
	//int16_t yMin 2
	//int16_t xMax 2
	//int16_t yMax 2

	//访问Glyph首部的numberOfContours成员
	if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
		return TTINVALIEFONTFORMAT;
	int16_t numberOfContours = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
	pGlyphData += 2;

	//如果numberOfContours>0,那么Glyph为Simple
	if (numberOfContours > 0)
	{
		TTGlyphSimple *pGlyphSimple = static_cast<TTGlyphSimple *>(g_pAlloc(sizeof(TTGlyphSimple)));
		if (pGlyphSimple == nullptr)
			return TTMEMORYALLOCFAILED;
		pGlyphSimple->bIsSimple = true;
		pGlyphSimple->contourNumber = static_cast<uint16_t>(numberOfContours);
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphSimple->bound.xMin = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphSimple->bound.yMin = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphSimple->bound.xMax = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphSimple->bound.yMax = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		
		//GlyphSimple数据
		//uint16_t endPtsOfContours[numberOfContours]
		//uint16_t instructionLength
		//uint8 instructions[instructionLength]
		//uint8_t flags[ ]
		//uint8_t/int16_t xCoordinate[ ]
		//uint8_t/int16_t yCoordinate[ ] 
		
		//访问GlyphSimple数据的endPtsOfContours成员
		pGlyphSimple->pContourEndPointerIndex = static_cast<uint16_t *>(g_pAlloc(sizeof(uint16_t)*pGlyphSimple->contourNumber));
		if (pGlyphSimple->pContourEndPointerIndex == nullptr)
			return TTMEMORYALLOCFAILED;
		for (uint16_t i = 0U; i < pGlyphSimple->contourNumber; ++i)
		{
			if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			pGlyphSimple->pContourEndPointerIndex[i] = NetWorkToHost(*reinterpret_cast<uint16_t *>(pGlyphData));
			pGlyphData += 2;
		}
		
		//endPtsOfContours中的最后一个索引加1是字形中的顶点数
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphSimple->pointNumber = pGlyphSimple->pContourEndPointerIndex[pGlyphSimple->contourNumber - 1] + 1;
		pGlyphSimple->pPoint = static_cast<TTGlyphSimplePoint *>(g_pAlloc(sizeof(TTGlyphSimplePoint)*pGlyphSimple->pointNumber));
		if (pGlyphSimple->pPoint == nullptr)
			return TTMEMORYALLOCFAILED;
		
		//instructionLength和instructions
		pGlyphData += (2 + NetWorkToHost(*reinterpret_cast<uint16_t *>(pGlyphData)));

		//访问GlyphSimple数据的flags成员并暂且缓存到TTGlyphPointer的bIsOnCurve中
		for (uint16_t i = 0U; i < pGlyphSimple->pointNumber; ++i)
		{
			if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			pGlyphSimple->pPoint[i].bIsOnCurve = *pGlyphData;
			//如果设置了0X8U标志，那么下一个flags表示在逻辑上字形中有多少个顶点的flags与上一个flags相同，不占flags成员中的存储空间
			if ((*pGlyphData) & 8U)
			{
				uint8_t flagcopysrc = *pGlyphData;
				++pGlyphData;
				if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				for (unsigned j = 0U; j<*pGlyphData; ++j)
				{
					++i;
					pGlyphSimple->pPoint[i].bIsOnCurve = flagcopysrc;
				}
				++pGlyphData;
			}
			else
				++pGlyphData;
		}

		//xCoordinate和yCoordinate表示相对于字形中上一个顶点的偏移，第一个顶点相对于0
		
		//访问GlyphSimple数据的xCoordinate成员
		int16_t offsetsrc = 0U;
		for (uint16_t i = 0U; i < pGlyphSimple->pointNumber; ++i)
		{
			//如果flags设置了0X2U标志，那么xCoordinate为uint8_t
			if (pGlyphSimple->pPoint[i].bIsOnCurve & 2U)
			{
				//如果flags设置了0X10U标志，那么表示xCoordinate为正
				if (pGlyphSimple->pPoint[i].bIsOnCurve & 0X10U)
				{
					if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc += *pGlyphData;
					pGlyphSimple->pPoint[i].x = offsetsrc;
				}
				//否则xCoordinate为负
				else
				{
					if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc -= *pGlyphData;
					pGlyphSimple->pPoint[i].x = offsetsrc;
				}
				++pGlyphData;
			}
			else
			{
				//如果flags设置了0X10U标志，那么表示在逻辑上xCoordinate为0，但不占用xCoordinate成员中的存储空间
				if (pGlyphSimple->pPoint[i].bIsOnCurve & 0X10U)
				{
					pGlyphSimple->pPoint[i].x = offsetsrc;
				}
				//否则xCoordinate为int16_t
				else
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc += NetWorkToHost((*reinterpret_cast<int16_t *>(pGlyphData)));
					pGlyphSimple->pPoint[i].x = offsetsrc;
					pGlyphData += 2;
				}
			}
		}

		//访问GlyphSimple数据的yCoordinate成员
		offsetsrc = 0U;
		for (uint16_t i = 0U; i < pGlyphSimple->pointNumber; ++i)
		{
			//如果flags设置了0X4U标志，那么yCoordinate为uint8_t
			if (pGlyphSimple->pPoint[i].bIsOnCurve & 4U)
			{
				//如果flags设置了0X20U标志，那么表示yCoordinate为正
				if (pGlyphSimple->pPoint[i].bIsOnCurve & 0X20U)
				{
					if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc += *pGlyphData;
					pGlyphSimple->pPoint[i].y = offsetsrc;
				}
				//否则yCoordinate为负
				else
				{
					if ((pGlyphData + 1) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc -= *pGlyphData;
					pGlyphSimple->pPoint[i].y = offsetsrc;
				}
				++pGlyphData;
			}
			else
			{
				//如果flags设置了0X20U标志，那么表示yCoordinate为0，但不占用yCoordinate中的存储空间
				if (pGlyphSimple->pPoint[i].bIsOnCurve & 0X20U)
				{
					pGlyphSimple->pPoint[i].y = offsetsrc;
				}
				//否则yCoordinate为int16_t
				else
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					offsetsrc += NetWorkToHost((*reinterpret_cast<int16_t *>(pGlyphData)));
					pGlyphSimple->pPoint[i].y = offsetsrc;
					pGlyphData += 2;
				}
			}
		}

		for (unsigned i = 0U; i < pGlyphSimple->pointNumber; ++i)
		{
			//如果flags设置了0X1U标志，那么表示OnCurve成立
			pGlyphSimple->pPoint[i].bIsOnCurve = ((pGlyphSimple->pPoint[i].bIsOnCurve & 1U) != 0);
		}

		*phGlyph = pGlyphSimple;
	}
	else
	{
		TTGlyphCompound *pGlyphCompound = static_cast<TTGlyphCompound *>(g_pAlloc(sizeof(TTGlyphCompound)));
		if (pGlyphCompound == nullptr)
			return TTMEMORYALLOCFAILED;
		pGlyphCompound->bIsSimple = false;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphCompound->bound.xMin = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphCompound->bound.yMin = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphCompound->bound.xMax = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;
		if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
			return TTINVALIEFONTFORMAT;
		pGlyphCompound->bound.yMax = NetWorkToHost(*reinterpret_cast<int16_t *>(pGlyphData));
		pGlyphData += 2;

		//GlyphCompound数据
		//struct{
		//uint16 flags 
		//uint16 glyphID
		//transition相关数据
		//rotationscale相关数据
		//}component[ ]
		TTGlyphCompoundChild temp[4];//这里假定复合字形中的分量不会超过4，超过4的分量会被简单地丢弃，实际中也很少超过4
		
		pGlyphCompound->childNumber = 0U;
		uint16_t bMoreComponents = true;
		while (bMoreComponents&&pGlyphCompound->childNumber < 4)
		{
			if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			uint16_t flags = *reinterpret_cast<uint16_t *>(pGlyphData);
			pGlyphData += 2;
			if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
				return TTINVALIEFONTFORMAT;
			temp[pGlyphCompound->childNumber].glyphID= *reinterpret_cast<uint16_t *>(pGlyphData);
			pGlyphData += 2;
			//如果flags设置了0X20U标志，那么表示还有更多的分量
			bMoreComponents = flags & 0X20U;
			//如果flags设置了0X20U标志，那么表示平移用向量表示
			if (flags & 0X2U)
			{
				temp[pGlyphCompound->childNumber].bIsTransitionVector = true;
				//如果flags设置了0X4U标志,那么表示RoundToGrid，仅在平移用向量表示时有意义
				temp[pGlyphCompound->childNumber].bIsTransitionRoundToGrid = flags & 0X4U;
				//如果flags设置了0X1U标志,那么transition相关数据为int16_t x;int16_t y;
				if (flags & 0X1U)
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.vector.x= *reinterpret_cast<int16_t *>(pGlyphData);
					pGlyphData += 2;
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.vector.y = *reinterpret_cast<int16_t *>(pGlyphData);
					pGlyphData += 2;
				}
				//否则transition相关数据为int8_t x;int8_t y;
				else
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.vector.x = *reinterpret_cast<int8_t *>(pGlyphData);
					++pGlyphData;
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.vector.y = *reinterpret_cast<int8_t *>(pGlyphData);
					++pGlyphData;
				}
			}
			//否则表示平移用匹配表示
			else
			{
				temp[pGlyphCompound->childNumber].bIsTransitionVector = false;
				//如果flags设置了0X1U标志,那么transition相关数据为uint16_t ParentPointerIndex;uint16_t ChildPointerIndex;
				if (flags & 0X1U)
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.match.ParentPointerIndex = *reinterpret_cast<uint16_t *>(pGlyphData);
					pGlyphData += 2;
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.match.ChildPointerIndex = *reinterpret_cast<uint16_t *>(pGlyphData);
					pGlyphData += 2;
				}
				//否则transition相关数据为uint8_t ParentPointerIndex;uint8_t ChildPointerIndex;
				else
				{
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.match.ParentPointerIndex = *reinterpret_cast<uint8_t *>(pGlyphData);
					++pGlyphData;
					if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
						return TTINVALIEFONTFORMAT;
					temp[pGlyphCompound->childNumber].transition.match.ChildPointerIndex = *reinterpret_cast<uint8_t *>(pGlyphData);
					++pGlyphData;
				}
			}
			
			//如果flags设置了0X8U标志，那么rotationscale相关数据为int16_t（实际上是整数部分2位小数部分14位的定点数）
			if (flags & 0X8U)
			{
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				float scale = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
				temp[pGlyphCompound->childNumber].rotationscale._11 = scale;
				temp[pGlyphCompound->childNumber].rotationscale._12 = 0.0f;
				temp[pGlyphCompound->childNumber].rotationscale._21 = 0.0f;
				temp[pGlyphCompound->childNumber].rotationscale._22 = scale;

			}
			//如果flags设置了0X40U标志，那么rotationscale相关数据为int16_t _11;int16_t _22;（实际上是整数部分2位小数部分14位的定点数）
			else if (flags & 0X40U)
			{
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._11 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
				temp[pGlyphCompound->childNumber].rotationscale._12 = 0.0f;
				temp[pGlyphCompound->childNumber].rotationscale._21 = 0.0f;
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._22 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
			}
			//如果flags设置了0X80U标志，那么rotationscale相关数据为int16_t _11;int16_t _12;int16_t _21;int16_t _22;（实际上是整数部分2位小数部分14位的定点数）
			else if (flags & 0X80U)
			{
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._11 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._12 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._21 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
				if ((pGlyphData + 2) > static_cast<TTFont *>(hFont)->pFontDataEnd)
					return TTINVALIEFONTFORMAT;
				temp[pGlyphCompound->childNumber].rotationscale._22 = static_cast<float>(*reinterpret_cast<int16_t *>(pGlyphData)) / 16384.0f;
				pGlyphData += 2;
			}
			//否则rotationscale相关为空
			else
			{
				temp[pGlyphCompound->childNumber].rotationscale._11 = 1.0f;
				temp[pGlyphCompound->childNumber].rotationscale._12 = 0.0f;
				temp[pGlyphCompound->childNumber].rotationscale._21 = 0.0f;
				temp[pGlyphCompound->childNumber].rotationscale._22 = 1.0f;
			}
			++pGlyphCompound->childNumber;
		};
		pGlyphCompound->pChild = static_cast<TTGlyphCompoundChild *>(g_pAlloc(sizeof(TTGlyphSimplePoint)*pGlyphCompound->childNumber));
		if (pGlyphCompound->pChild == nullptr)
			return TTMEMORYALLOCFAILED;
		for (uint16_t i = 0U; i < pGlyphCompound->childNumber; ++i)
		{
			pGlyphCompound->pChild[i] = temp[i];
		}
		*phGlyph = pGlyphCompound;
	}
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphFree(TTHGlyph hGlyph)
{
	if (g_pFree == nullptr)
		return TTNOTINITIALISED;
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	g_pFree(hGlyph);
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphIsSimple(TTHGlyph hGlyph, uint8_t *pbIsSimple)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	*pbIsSimple = static_cast<TTGlyph *>(hGlyph)->bIsSimple;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphGetBound(TTHGlyph hGlyph, const TTRect **ppBound)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (ppBound == nullptr)
		return TTINVALIDARGUMENTS;
	*ppBound = &static_cast<TTGlyph *>(hGlyph)->bound;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphSimpleGetContourNumber(TTHGlyph hGlyph, uint16_t *pContourNumber)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (!static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*pContourNumber = static_cast<TTGlyphSimple *>(hGlyph)->contourNumber;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphSimpleGetContourEndPointerIndex(TTHGlyph hGlyph, const uint16_t **ppContourEndPointerIndexArray)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (!static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*ppContourEndPointerIndexArray = static_cast<TTGlyphSimple *>(hGlyph)->pContourEndPointerIndex;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphSimpleGetPointNumber(TTHGlyph hGlyph, uint16_t *pPointNumber)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (!static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*pPointNumber = static_cast<TTGlyphSimple *>(hGlyph)->pointNumber;
	return TTSUCCESS;
}
extern"C" uint8_t __stdcall TTGlyphSimpleGetPointArray(TTHGlyph hGlyph, const TTGlyphSimplePoint **pPointArray)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (!static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*pPointArray = static_cast<TTGlyphSimple *>(hGlyph)->pPoint;
	return TTSUCCESS;
}

extern"C" uint8_t __stdcall TTGlyphCompoundGetChildNumber(TTHGlyph hGlyph, uint16_t *pChildNumber)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*pChildNumber = static_cast<TTGlyphCompound *>(hGlyph)->childNumber;
	return TTSUCCESS;
}
extern"C" uint8_t __stdcall TTGlyphCompoundGetChildArray(TTHGlyph hGlyph, const TTGlyphCompoundChild **ppChildArray)
{
	if (hGlyph == nullptr)
		return TTINVALIDARGUMENTS;
	if (static_cast<TTGlyph *>(hGlyph)->bIsSimple)
		return TTINVALIDARGUMENTS;
	*ppChildArray = static_cast<TTGlyphCompound *>(hGlyph)->pChild;
	return TTSUCCESS;
}
