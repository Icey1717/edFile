#include "edFile.h"
#include "EdenLib/edSys/sources/EdSystem.h"
#include "edMem.h"

#include "ps2/_edFileFilerCDVD.h"
#include "edVideo/VideoD.h"
#include "ps2/_edFileFilerMCard.h"
#include "../include/edFileNoWaitStack.h"

byte edFileHandleTable[16];
edFILEH edFileHandleData[16];

edSysHandlerFile edFileHandlers;

edCFileFind edFileFind;

void* GetInternalData_0025b2e0(edFILEH* pFile)
{
	void* pvVar1;

	pvVar1 = (void*)0x0;
	if ((pFile != (edFILEH*)0x0) &&
		(pvVar1 = pFile->pFileData, pvVar1 == (void*)0x0)) {
		pvVar1 = (void*)0x0;
	}
	return pvVar1;
}

char* edFileOpen(char* filePath, uint* outSize, uint flags)
{
	byte bVar1;
	edFILEH* pFile;
	edCFiler* pFiler;
	uint uVar3;
	char* pReadBuffer;
	byte* pbVar5;
	int iVar6;
	char acStack512[512];

	MY_LOG("MessageFile::edFileOpen {}\n", filePath);

	*outSize = 0;
	pFile = edFileOpen(filePath, flags | 1);
	if (pFile == (edFILEH*)0x0) {
		pReadBuffer = (char*)0x0;
	}
	else {
		pFiler = edFileGetFiler(acStack512, filePath, 0);
		if (pFiler == (edCFiler*)0x0) {
			pReadBuffer = (char*)0x0;
		}
		else {
			uVar3 = 0;
			if ((pFile->openFlags & 6) == 0) {
				uVar3 = (pFile->fileInfo).fileSize;
				//peVar7 = peVar2->pVTable;
			}
			else {
				//peVar7 = peVar2->pVTable;
			}

			*outSize = pFiler->getalignedsize(uVar3);

			pReadBuffer = (char*)edMemAllocAlignBoundary(edFileLoadInfo.heap, *outSize, (uint)edFileLoadInfo.align, (uint)edFileLoadInfo.offset);
			edFileLoadInfo.offset = 0;
			edFileLoadInfo.heap = TO_HEAP(H_MAIN);
			edFileLoadInfo.align = 0x40;

			pFiler->getnowaitfilestack()->AddFileRead(pFile, pReadBuffer, *outSize);
			pFiler->getnowaitfilestack()->AddFileClose(pFile);

			if ((pFile->openFlags & 8) == 0) {
				edFileNoWaitStackFlush(pFile->pOwningFiler);
				iVar6 = 0;
				do {
					if (&edFileHandleData[iVar6] == pFile) {
						bVar1 = edFileHandleTable[iVar6];
						goto LAB_0025bac8;
					}
					iVar6 = iVar6 + 1;
				} while (iVar6 < 0x10);
				bVar1 = 0;
			LAB_0025bac8:
				if (bVar1 == 0) {
					bVar1 = 1;
				}
				else {
					bVar1 = pFile->bInUse;
				}
			}
			else {
				bVar1 = 1;
			}
			if (bVar1 == 0) {
				*outSize = 0;
				edMemFree(pReadBuffer);
				pReadBuffer = (char*)0x0;
			}
		}
	}
	return pReadBuffer;
}

edFILEH* edFileOpen(char* szFilePath, uint flags)
{
	bool bOpenSuccess;
	edCFiler* pFiler;
	edFILEH* pFile;
	int bankIndex;
	edFILEH* pDebugBank;
	char szFilerPath[512];

	MY_LOG("edFileOpen {}\n", szFilePath);

	pFiler = edFileGetFiler(szFilerPath, szFilePath, 0);

	MY_LOG("edFileOpen Filer Path {}\n", szFilerPath);

	if (pFiler == (edCFiler*)0x0) {
		pFile = (edFILEH*)0x0;
	}
	else {
		if (((flags & 8) != 0) && ((pFiler->flags & 0x20) == 0)) {
			flags = flags & 0xfffffff7;
		}
		if ((((flags & 2) == 0) || (pFile = (edFILEH*)0x0, (pFiler->flags & 2) != 0)) &&
			(((flags & 4) == 0 || (pFile = (edFILEH*)0x0, (pFiler->flags & 4) != 0)))) {
			if (((flags & 8) == 0) || ((pFiler->flags & 0x80) != 0)) {
				edFileNoWaitStackFlush(pFiler);
			}
			bankIndex = 0;
			do {
				if (edFileHandleTable[bankIndex] == 0) {
					edFileHandleTable[bankIndex] = 1;
					pDebugBank = &edFileHandleData[bankIndex];
					memset(pDebugBank, 0, sizeof(edFILEH));
					goto LAB_0025c168;
				}
				bankIndex = bankIndex + 1;
			} while (bankIndex < 0x10);
			pDebugBank = (edFILEH*)0x0;
		LAB_0025c168:
			pFile = (edFILEH*)0x0;
			if (pDebugBank != (edFILEH*)0x0) {
				pDebugBank->pOwningFiler = pFiler;
				pDebugBank->openFlags = flags;
				/* May call edCFiler_CDVD_open */
				bOpenSuccess = pFiler->open(pDebugBank, szFilerPath);
				if (bOpenSuccess == false) {
					bankIndex = 0;
					do {
						if (&edFileHandleData[bankIndex] == pDebugBank) {
							edFileHandleTable[bankIndex] = 0;
							memset(&edFileHandleData[bankIndex], 0, sizeof(edFILEH));
							break;
						}
						bankIndex = bankIndex + 1;
					} while (bankIndex < 0x10);
					pFile = (edFILEH*)0x0;
				}
				else {
					pDebugBank->bInUse = 1;
					pFile = pDebugBank;
				}
			}
		}
	}
	return pFile;
}

bool edFileMakeDir(char* szPath)
{
	bool bSuccess;
	edCFiler* pFiler;
	char szFilerPath[512];

	pFiler = edFileGetFiler(szFilerPath, szPath, 1);
	if (pFiler == (edCFiler*)0x0) {
		bSuccess = false;
	}
	else {
		bSuccess = pFiler->mkdir(szFilerPath);
	}

	return bSuccess;
}

uint edFileLoadSize(edFILEH* pDebugBank)
{
	uint uVar1;

	uVar1 = 0;
	if ((pDebugBank->openFlags & 6) == 0) {
		uVar1 = (pDebugBank->fileInfo).fileSize;
	}
	return uVar1;
}


int edFileIsIdle(char* szPath, int param_2)
{
	edCFiler* pFiler;
	int iVar1;
	char szFilerPath[512];

	pFiler = edFileGetFiler(szFilerPath, szPath, 1);
	if (pFiler == (edCFiler*)0x0) {
		iVar1 = 0;
	}
	else {
		iVar1 = pFiler->isidle(szFilerPath, param_2);
	}

	return iVar1;
}

bool edFileGetFreeSpace(char* szPath, ulong* pFreeBytes, ulong* pFreeClusters, byte* param_4)
{
	bool bResult;
	edCFiler* pFiler;
	char szFilerPath[512];

	pFiler = edFileGetFiler(szFilerPath, szPath, 1);
	if (pFiler == (edCFiler*)0x0) {
		bResult = false;
	}
	else {
		bResult = pFiler->getfreespace(szFilerPath, pFreeBytes, pFreeClusters, param_4);
	}

	return bResult;
}

bool edFileRead(edFILEH* pFile, char* pDst, uint size)
{
	byte bVar2;
	bool bSuccess;
	int handleIndex;

	pFile->pOwningFiler->getnowaitfilestack()->AddFileRead(pFile, pDst, size);

	if ((pFile->openFlags & 8) == 0) {
		edFileNoWaitStackFlush(pFile->pOwningFiler);
		handleIndex = 0;
		do {
			if (&edFileHandleData[handleIndex] == pFile) {
				bVar2 = edFileHandleTable[handleIndex];
				goto LAB_0025bf20;
			}
			handleIndex = handleIndex + 1;
		} while (handleIndex < 0x10);

		bVar2 = 0;
	LAB_0025bf20:
		if (bVar2 == 0) {
			bSuccess = true;
		}
		else {
			bSuccess = pFile->bInUse;
		}
	}
	else {
		bSuccess = true;
	}

	return bSuccess;
}


bool edFileSeek(edFILEH* pFile, uint seekOffset, ESeekMode mode)
{
	byte bVar2;
	bool bSuccess;
	byte* pbVar4;
	int iVar5;

	if (mode == ED_SEEK_END) {
		seekOffset = (pFile->fileInfo).fileSize + seekOffset;
	}
	else {
		if (mode == ED_SEEK_CUR) {
			seekOffset = pFile->seekOffset + seekOffset;
		}
		else {
			if (mode != ED_SEEK_SET) {
				seekOffset = 0;
			}
		}
	}

	pFile->pOwningFiler->getnowaitfilestack()->AddFileSeek(pFile, seekOffset);

	if ((pFile->openFlags & 8) == 0) {
		edFileNoWaitStackFlush(pFile->pOwningFiler);
		iVar5 = 0;
		do {
			if (&edFileHandleData[iVar5] == pFile) {
				bVar2 = edFileHandleTable[iVar5];
				goto LAB_0025b788;
			}
			iVar5 = iVar5 + 1;
		} while (iVar5 < 0x10);
		bVar2 = 0;
	LAB_0025b788:
		if (bVar2 == 0) {
			bSuccess = true;
		}
		else {
			bSuccess = (bool)pFile->bInUse;
		}
	}
	else {
		bSuccess = true;
	}

	return bSuccess;
}

bool edFileFlush(void)
{
	edFileNoWaitStackFlush((edCFiler*)0x0);
	return true;
}

bool edFileWrite(edFILEH* pFile, void* pData, uint size)
{
	byte bVar1;
	bool bSuccess;
	byte* pbVar2;
	int iVar3;

	pFile->pOwningFiler->getnowaitfilestack()->AddFileWrite(pFile, pData, size);

	if ((pFile->openFlags & 8) == 0) {
		edFileNoWaitStackFlush(pFile->pOwningFiler);

		iVar3 = 0;
		do {
			if (&edFileHandleData[iVar3] == pFile) {
				bVar1 = edFileHandleTable[iVar3];
				goto LAB_0025b660;
			}

			iVar3 = iVar3 + 1;
		} while (iVar3 < 0x10);

		bVar1 = 0;

	LAB_0025b660:
		if (bVar1 == 0) {
			bSuccess = true;
		}
		else {
			bSuccess = pFile->bInUse;
		}
	}
	else {
		bSuccess = true;
	}

	return bSuccess;
}

bool edFileClose(edFILEH* pFile)
{
	byte bVar2;
	bool bVar1;
	edCFileNoWaitStack* peVar3;
	int iVar5;

	pFile->pOwningFiler->getnowaitfilestack()->AddFileClose(pFile);

	if ((pFile->openFlags & 8) == 0) {
		edFileNoWaitStackFlush(pFile->pOwningFiler);

		iVar5 = 0;
		do {
			if (&edFileHandleData[iVar5] == pFile) {
				bVar2 = edFileHandleTable[iVar5];
				goto LAB_0025b4b0;
			}
			iVar5 = iVar5 + 1;
		} while (iVar5 < 0x10);
		bVar2 = 0;
	LAB_0025b4b0:
		if (bVar2 == 0) {
			bVar1 = true;
		}
		else {
			bVar1 = (bool)pFile->bInUse;
		}
	}
	else {
		bVar1 = true;
	}
	return bVar1;
}

bool edFileCmdBreak(edFILEH* pFile)
{
	return pFile->pOwningFiler->cmdbreak();
}

const char* sz_edCBankBuffer_Wait_0042fb70 = "edCBankBufferEntry::file_access: Wait for end of previous loading operation \n";

uint edFileGetSize(edFILEH* param_1)
{
	uint uVar1;

	uVar1 = 0;
	if ((param_1->openFlags & 6) == 0) {
		uVar1 = (param_1->fileInfo).fileSize;
	}
	return uVar1;
}

void _edFileInit(void)
{
	byte bVar1;

	//if ((DAT_00448fd1 != '\0') && (bVar1 = edPsx2IsNetConnection(), bVar1 != 0)) {
	//	edFileSetPath(s_<net>_00431458);
	//	g_NetFilePtr_00448fd4 = (undefined*)edFileOpen(PTR_s_NetFile.log_00448930, 6);
	//	edSysHandlers::edSysHandlersAdd(edSysHandlersNodeParent_0048cee0.pNodeTable, edSysHandlersNodeParent_0048cee0.pTypeArray_0x4, (long)g_SysHandlersMainMaxEventID_0048cf10, New_Name, FUN_00260ae0, 1, 0);
	//}
	_edFileMCardAddFiler();
	//_edFileHostPcAddFiler();
	_edFileCDVDAddFiler();
	//_edFileNetAddFiler();
	//_edFileHDDAddFiler();
	return;
}

void edFileGetFiler(int, int, char*)
{
	edFileNoWaitStackFlush();
}

edFileLoadConfig edFileLoadInfo;

bool edFileInit(void)
{
	edCFiler* piVar2;
	bool bSuccess;
	bool bVar3;

	//edFileDebugInit();
	_edFileInit();
	//EdFileBase::edFileMemAddFiler();
	piVar2 = edFilerList.get_root();
	bVar3 = true;
	for (; piVar2 != (edCFiler*)0x0; piVar2 = piVar2->pNextEd) {
		bSuccess = piVar2->initialize();
		if (bSuccess == false) {
			bVar3 = false;
		}
	}

	edFileLoadInfo.offset = 0;
	edFileLoadInfo.heap = TO_HEAP(H_MAIN);
	edFileLoadInfo.align = 0x40;

	// Also pump IO during our render.
	if (edSysHandlersAdd(edVideoHandlers.nodeParent, edVideoHandlers.entries, edVideoHandlers.maxEventID, ED_HANDLER_VIDEO_RENDER, edFileGetFiler, 3, 1) == false) {
		bVar3 = false;
	}

	return bVar3;
}

bool edFileGetPhysicalFileName(char* filePathOut, char* filePathIn)
{
	bool uVar1;
	edCFiler* peVar1;
	char acStack512[512];

	if ((filePathOut == (char*)0x0) || (filePathIn == (char*)0x0)) {
		uVar1 = false;
	}
	else {
		*filePathOut = '\0';
		peVar1 = edFileGetFiler(acStack512, filePathIn, 0);
		if (peVar1 == (edCFiler*)0x0) {
			uVar1 = false;
		}
		else {
			uVar1 = peVar1->get_physical_filename(filePathOut, acStack512);
		}
	}
	return uVar1;
}

bool edFileFindFirst(edFILE_INFO* pFileInfo, char* pName)
{
	bool bResult;

	if ((edFileFind.bOpen == 0) && (pFileInfo != (edFILE_INFO*)0x0)) {
		memset(&edFileFind, 0, sizeof(edCFileFind));
		edFileFind.bOpen = 1;
		edFileFind.pFileInfo = pFileInfo;
		memset(pFileInfo, 0, sizeof(edFILE_INFO));

		edFileFind.pFiler = edFileGetFiler(edFileFind.szSlotPathA, pName, 0);
		if (edFileFind.pFiler == (edCFiler*)0x0) {
			bResult = false;
		}
		else {
			bResult = edFileFind.pFiler->findfile(&edFileFind, ED_FIND_FIRST);
			if (bResult == false) {
				if (edFileFind.bOpen != 0) {
					if (edFileFind.pFiler != (edCFiler*)0x0) {
						edFileFind.bOpen = 0;
						edFileFind.pFiler->findfile(&edFileFind, ED_FIND_CLOSE);
					}
				}
				bResult = false;
			}
			else {
				bResult = true;
			}
		}
	}
	else {
		bResult = false;
	}

	return bResult;
}

bool edFileFindClose(void)
{
	if ((edFileFind.bOpen != 0) && (edFileFind.pFiler != (edCFiler*)0x0)) {
		edFileFind.bOpen = 0;
		return edFileFind.pFiler->findfile(&edFileFind, ED_FIND_CLOSE);
	}

	return false;
}

bool edFileDelete(char* szPath)
{
	bool bResult;
	edCFiler* pFiler;
	char szFilerPath[512];

	pFiler = edFileGetFiler(szFilerPath, szPath, 0);
	if (pFiler == (edCFiler*)0x0) {
		bResult = false;
	}
	else {
		bResult = pFiler->erase(szFilerPath);
	}
	return bResult;
}

edFILE_INFO* GetFileInfoFromFile(edFILEH* pFile)
{
	edFILE_INFO* pFileInfo;

	pFileInfo = &pFile->fileInfo;

	if ((pFile->openFlags & 6) != 0) {
		pFileInfo = (edFILE_INFO*)0x0;
	}

	return pFileInfo;
}