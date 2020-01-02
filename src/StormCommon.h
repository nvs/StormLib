/*
 * StormCommon.h
 *
 * Copyright (c) Ladislav Zezula
 */

#ifndef __STORMCOMMON_H__
#define __STORMCOMMON_H__

#include "StormLib.h"
#include "StormPort.h"

#define MPQ_HASH_TABLE_INDEX    0x000
#define MPQ_HASH_NAME_A         0x100
#define MPQ_HASH_NAME_B         0x200

ULONGLONG HashStringJenkins(const char * szFileName);

TMPQHash * GetFirstHashEntry(TMPQArchive * ha, const char * szFileName);
TMPQHash * GetNextHashEntry(TMPQArchive * ha, TMPQHash * pFirstHash, TMPQHash * pPrevHash);

TFileEntry * GetFileEntryExact(TMPQArchive * ha, const char * szFileName, LCID lcLocale, LPDWORD PtrHashIndex);

#endif
