#ifndef TTF_H
#define TTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TTSUCCESS 0
#define TTNOTINITIALISED 1
#define TTINVALIDARGUMENTS 2
#define TTMEMORYALLOCFAILED 3
#define TTINVALIEFONTFORMAT 4


uint8_t __stdcall TTLibraryInit(void *(__stdcall *const pAlloc)(size_t size), void(__stdcall *const pFree)(void *p));
uint8_t __stdcall TTLibraryFree();

typedef struct TTHFont__ {} *TTHFont;
uint8_t __stdcall TTFontLoadFromMemory(const void *pData, size_t size, TTHFont *phFont);
uint8_t __stdcall TTFontFree(TTHFont hFont);

typedef struct
{
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;
}TTRect;
uint8_t __stdcall TTFontGetBound(TTHFont hFont, const TTRect **ppBound);


uint8_t __stdcall TTFontIsUCS2Supported(TTHFont hFont, uint8_t *pbIsSupported);
uint8_t __stdcall TTFontGetGlyphIDUCS2(TTHFont hFont, uint16_t UCS2CharCode, uint16_t *pGlyphID);
uint8_t __stdcall TTFontIsUCS4Supported(TTHFont hFont, uint8_t *pbIsSupported);
uint8_t __stdcall TTFontGetGlyphIDUCS4(TTHFont hFont, uint32_t UCS4CharCode, uint16_t *pGlyphID);

typedef struct TTHGlyph__ {} *TTHGlyph;
uint8_t __stdcall TTGlyphInit(TTHFont hFont, uint16_t glyphID, TTHGlyph *phGlyph);
uint8_t __stdcall TTGlyphFree(TTHGlyph hGlyph);

uint8_t __stdcall TTGlyphGetBound(TTHGlyph hGlyph, const TTRect **ppBound);
uint8_t __stdcall TTGlyphIsSimple(TTHGlyph hGlyph, uint8_t *pbIsSimple);

uint8_t __stdcall TTGlyphSimpleGetContourNumber(TTHGlyph hGlyph, uint16_t *pContourNumber);
uint8_t __stdcall TTGlyphSimpleGetContourEndPointerIndex(TTHGlyph hGlyph, const uint16_t **ppContourEndPointerIndexArray);

typedef struct
{
	uint8_t bIsOnCurve;
	int16_t x;
	int16_t y;
}TTGlyphSimplePoint;
uint8_t __stdcall TTGlyphSimpleGetPointNumber(TTHGlyph hGlyph, uint16_t *pPointNumber);
uint8_t __stdcall TTGlyphSimpleGetPointArray(TTHGlyph hGlyph, const TTGlyphSimplePoint **pPointArray);

typedef struct
{
	union
	{
		struct
		{
			float _11, _12;
			float _21, _22;
		};
		float m[2][2];
	};
}TTFLOAT2X2;
typedef struct
{
	uint16_t glyphID;
	uint8_t bIsTransitionVector;
	uint8_t bIsTransitionRoundToGrid;//仅适用于Vector
	union
	{
		struct
		{
			int16_t x;
			int16_t y;
		}vector;
		struct
		{
			uint16_t ParentPointerIndex;
			uint16_t ChildPointerIndex;
		}match;
	}transition;
	TTFLOAT2X2 rotationscale;
}TTGlyphCompoundChild;
uint8_t __stdcall TTGlyphCompoundGetChildNumber(TTHGlyph hGlyph, uint16_t *pChildNumber);
uint8_t __stdcall TTGlyphCompoundGetChildArray(TTHGlyph hGlyph, const TTGlyphCompoundChild **ppChildArray);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
