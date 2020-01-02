/*
 * SBaseFileTable.cpp
 *
 * Copyright (c) Ladislav Zezula
 */

#include "StormCommon.h"
#include "StormLib.h"
#include "StormPort.h"
#include <assert.h>
#include <stddef.h>

// Returns a hash table entry in the following order:
// 1) A hash table entry with the preferred locale
// 2) NULL
static TMPQHash * GetHashEntryExact(TMPQArchive * ha, const char * szFileName, LCID lcLocale)
{
    TMPQHash * pFirstHash = GetFirstHashEntry(ha, szFileName);
    TMPQHash * pHash = pFirstHash;

    // Parse the found hashes
    while(pHash != NULL)
    {
        // If the locales match, return it
        if(pHash->lcLocale == lcLocale)
            return pHash;

        // Get the next hash entry for that file
        pHash = GetNextHashEntry(ha, pFirstHash, pHash);
    }

    // Not found
    return NULL;
}

static DWORD GetFileIndex_Het(TMPQArchive * ha, const char * szFileName)
{
    TMPQHetTable * pHetTable = ha->pHetTable;
    ULONGLONG FileNameHash;
    DWORD StartIndex;
    DWORD Index;
    BYTE NameHash1;                 // Upper 8 bits of the masked file name hash

    // If there are no entries in the HET table, do nothing
    if(pHetTable->dwEntryCount == 0)
        return HASH_ENTRY_FREE;

    // Do nothing if the MPQ has no HET table
    assert(ha->pHetTable != NULL);

    // Calculate 64-bit hash of the file name
    FileNameHash = (HashStringJenkins(szFileName) & pHetTable->AndMask64) | pHetTable->OrMask64;

    // Split the file name hash into two parts:
    // NameHash1: The highest 8 bits of the name hash
    // NameHash2: File name hash limited to hash size
    // Note: Our file table contains full name hash, no need to cut the high 8 bits before comparison
    NameHash1 = (BYTE)(FileNameHash >> (pHetTable->dwNameHashBitSize - 8));

    // Calculate the starting index to the hash table
    StartIndex = Index = (DWORD)(FileNameHash % pHetTable->dwTotalCount);

    // Go through HET table until we find a terminator
    while(pHetTable->pNameHashes[Index] != HET_ENTRY_FREE)
    {
        // Did we find a match ?
        if(pHetTable->pNameHashes[Index] == NameHash1)
        {
            DWORD dwFileIndex = 0;

            // Get the file index
            GetBits(pHetTable->pBetIndexes, pHetTable->dwIndexSizeTotal * Index,
                                            pHetTable->dwIndexSize,
                                           &dwFileIndex,
                                            sizeof(DWORD));

            // Verify the FileNameHash against the entry in the table of name hashes
            if(dwFileIndex <= ha->dwFileTableSize && ha->pFileTable[dwFileIndex].FileNameHash == FileNameHash)
            {
                return dwFileIndex;
            }
        }

        // Move to the next entry in the HET table
        // If we came to the start index again, we are done
        Index = (Index + 1) % pHetTable->dwTotalCount;
        if(Index == StartIndex)
            break;
    }

    // File not found
    return HASH_ENTRY_FREE;
}

TFileEntry * GetFileEntryExact(TMPQArchive * ha, const char * szFileName, LCID lcLocale, LPDWORD PtrHashIndex)
{
    TMPQHash * pHash;
    DWORD dwFileIndex;

    // If the hash table is present, find the entry from hash table
    if(ha->pHashTable != NULL)
    {
        pHash = GetHashEntryExact(ha, szFileName, lcLocale);
        if(pHash != NULL && MPQ_BLOCK_INDEX(pHash) < ha->dwFileTableSize)
        {
            if(PtrHashIndex != NULL)
                PtrHashIndex[0] = (DWORD)(pHash - ha->pHashTable);
            return ha->pFileTable + MPQ_BLOCK_INDEX(pHash);
        }
    }

    // If we have HET table in the MPQ, try to find the file in HET table
    if(ha->pHetTable != NULL)
    {
        dwFileIndex = GetFileIndex_Het(ha, szFileName);
        if(dwFileIndex != HASH_ENTRY_FREE)
        {
            if(PtrHashIndex != NULL)
                PtrHashIndex[0] = HASH_ENTRY_FREE;
            return ha->pFileTable + dwFileIndex;
        }
    }

    // Not found
    return NULL;
}
