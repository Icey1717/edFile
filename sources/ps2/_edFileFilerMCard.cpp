#include "ps2/_edFileFilerMCard.h"
#include "edFilePath.h"
#include "edFile.h"

#ifdef PLATFORM_PS2
// MC Includes
#else
#include <filesystem>
#include <iostream>
#include <fstream>
#include <exception>
#endif
#include "edStr.h"

edCFiler_MemoryCard edFiler_MemoryCard;

bool edCFiler_MemoryCard::configure(uint)
{
	IMPLEMENTATION_GUARD();
}

bool edCFiler_MemoryCard::initialize()
{
	int iVar2;

	this->pDriveName_0x0 = "<MC>";

	set_default_unit("0:");

	this->flags = 0x2f;
	this->field_0x214 = 0;

	iVar2 = 0;
	MCSpaceStruct_0x10* peVar1 = this->aSpaceStructs;
	do {
		peVar1->port = iVar2;
		peVar1->slot = 0;
		peVar1->field_0x0 = 0;
		peVar1->bIsSyncIdle = 1;
		iVar2 = iVar2 + 1;
		peVar1 = peVar1 + 1;
	} while (iVar2 < 2);

	this->activeMemCardIndex = 0;

#ifdef PLATFORM_PS2
	return sceMcInit() == 0;
#else
	return true;
#endif
}

bool edCFiler_MemoryCard::terminate()
{
	IMPLEMENTATION_GUARD();
}

edCFileNoWaitStack* edCFiler_MemoryCard::getnowaitfilestack()
{
	return &this->field_0x23c;
}

bool edCFiler_MemoryCard::get_physical_filename(char* outFilePath, char* pathBuff)
{
	IMPLEMENTATION_GUARD();
}

#ifdef PLATFORM_WIN
static bool OpenFileWin(edFILEH* pFile, char* unformatedFilePath, MC_OpenFile* pOpenFile)
{
	uint edFileOpenMode;

	std::filesystem::path filePath(unformatedFilePath);

	edFileOpenMode = pFile->openFlags & 3;

	if (((pFile->openFlags & 2) != 0) && ((pFile->openFlags & 4) != 0)) {

	}

	// Try open the file with std C++ filesystem
	try {
		if (edFileOpenMode == 1) {
			// Read only
			pOpenFile->pStream = new std::ifstream(filePath, std::ios::binary);
		}
		else if (edFileOpenMode == 2) {
			// Write only
			pOpenFile->pStream = new std::ofstream(filePath, std::ios::binary);
		}
		else if (edFileOpenMode == 3) {
			// Read and write
			pOpenFile->pStream = new std::fstream(filePath, std::ios::in | std::ios::out | std::ios::binary);
		}
		else {
			// Invalid mode
			return false;
		}
	}
	catch (const std::exception& e) {
		// Failed to open the file
		pOpenFile->pStream = nullptr;
		return false;
	}

	if (pOpenFile->pStream == nullptr) {
		return false;
	}

	if (edFileOpenMode == 1) {
		std::ifstream* pStream = static_cast<std::ifstream*>(pOpenFile->pStream);
		if (!pStream->is_open()) {
			delete pStream;
			pOpenFile->pStream = nullptr;
			return false;
		}
	}
	else if (edFileOpenMode == 2) {
		std::ofstream* pStream = static_cast<std::ofstream*>(pOpenFile->pStream);
		if (!pStream->is_open()) {
			delete pStream;
			pOpenFile->pStream = nullptr;
			return false;
		}
	}
	else if (edFileOpenMode == 3) {
		std::fstream* pStream = static_cast<std::fstream*>(pOpenFile->pStream);
		if (!pStream->is_open()) {
			delete pStream;
			pOpenFile->pStream = nullptr;
			return false;
		}
	}

	return true;
}
#endif

#ifdef PLATFORM_PS2
bool _edFile_MakeSceOpenFlag(uint* pSceOpenFlags, uint edFileFlags)
{
	bool bSuccess;
	uint edFileOpenMode;

	edFileOpenMode = edFileFlags & 3;

	*pSceOpenFlags = 0;
	if (edFileOpenMode == 1) {
		*pSceOpenFlags = SCE_RDONLY;
	}
	else {
		if (edFileOpenMode == 2) {
			*pSceOpenFlags = SCE_WRONLY;
		}
		else {
			if (edFileOpenMode == 3) {
				*pSceOpenFlags = SCE_RDWR;
			}
		}
	}

	bSuccess = false;
	if (((edFileFlags & 2) != 0) && ((edFileFlags & 4) != 0)) {
		bSuccess = true;
		*pSceOpenFlags = *pSceOpenFlags | SCE_CREAT;
		*pSceOpenFlags = *pSceOpenFlags | 0x400;
	}

	return bSuccess;
}
#endif

#ifdef PLATFORM_WIN
static bool WinTranslateFileInfo(edFILE_INFO* pInfo, char* szPath)
{
	// Check if the file or directory exists
	try {
		const std::filesystem::path pathToCheck(szPath);

		if (std::filesystem::exists(pathToCheck)) {
			// Fill in edFILE_INFO using std::filesystem
			pInfo->create.sec = 0; // std::filesystem does not provide creation time directly
			pInfo->create.min = 0;
			pInfo->create.hour = 0;
			pInfo->create.day = 0;
			pInfo->create.month = 0;
			pInfo->create.year = 0;

			pInfo->modify.sec = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(pathToCheck).time_since_epoch()).count() % 60;
			pInfo->modify.min = (std::chrono::duration_cast<std::chrono::minutes>(std::filesystem::last_write_time(pathToCheck).time_since_epoch()).count() / 60) % 60;
			pInfo->modify.hour = (std::chrono::duration_cast<std::chrono::hours>(std::filesystem::last_write_time(pathToCheck).time_since_epoch()).count() / 3600) % 24;
			pInfo->modify.day = std::filesystem::last_write_time(pathToCheck).time_since_epoch().count() % 31 + 1; // Simplified
			pInfo->modify.month = (std::filesystem::last_write_time(pathToCheck).time_since_epoch().count() / 31) % 12 + 1; // Simplified
			pInfo->modify.year = 1970 + (std::filesystem::last_write_time(pathToCheck).time_since_epoch().count() / 365); // Simplified

			pInfo->fileSize = static_cast<uint>(std::filesystem::file_size(pathToCheck));
			pInfo->flags = std::filesystem::is_directory(pathToCheck) ? 0x10 : 0x20; // Directory or file
			edStrCopy(pInfo->name, pathToCheck.filename().string().c_str());

			return true;
		}
		else {
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error accessing file: " << e.what() << std::endl;
		return false;
	}
}
#endif

bool edCFiler_MemoryCard::open(edFILEH* pFile, char* szPath)
{
	char cVar1;
	bool bVar2;
	MCSpaceStruct_0x10* pSpaceStruct;
	edCFileNoWaitStack* pElementContainer;
	char* pcVar4;
	edFILE_INFO* pFileInfo;
	int iVar5;
	int iVar6;
	MC_OpenFile* pOpenFile;
#ifdef PLATFORM_PS2
	sceMcTblGetDir sStack1664;
#endif
	char local_620[512];
	char local_420[512];
	char local_220[524];
	int local_14;
	int local_10;
	int syncResult;
	uint sceOpenFlags;

	if ((pFile->openFlags & 6) == 0) {
		iVar5 = 0;
		do {
			if (this->field_0x218[iVar5] == 0) {
				this->field_0x218[iVar5] = 1;
				pOpenFile = this->field_0x21c + iVar5;
				memset(pOpenFile, 0, sizeof(MC_OpenFile));
				goto LAB_0025f648;
			}
			iVar5 = iVar5 + 1;
		} while (iVar5 < 4);

		pOpenFile = (MC_OpenFile*)0x0;

	LAB_0025f648:
		if (pOpenFile == (MC_OpenFile*)0x0) {
			return false;
		}

		edFilePathSplit((char*)0x0, local_620, (char*)0x0, (char*)0x0, szPath);

		if (local_620[0] + -0x30 < 2) {
			pSpaceStruct = this->aSpaceStructs + (local_620[0] + -0x30);
		}
		else {
			pSpaceStruct = (MCSpaceStruct_0x10*)0x0;
		}

		if (pSpaceStruct->field_0x0 == 0) {
			this->field_0x23c.AddFilerSync(this);
		}

		edFileNoWaitStackFlush(this);

		if (pSpaceStruct->field_0x0 == 0) {
			pSpaceStruct = (MCSpaceStruct_0x10*)0x0;
		}

		if (pSpaceStruct == (MCSpaceStruct_0x10*)0x0) {
			iVar6 = 0;

			MC_OpenFile* pOpen = this->field_0x21c;
			do {
				if (pOpen == pOpenFile) {
					this->field_0x218[iVar6] = 0;
					memset(this->field_0x21c + iVar6, 0, sizeof(MC_OpenFile));
					return false;
				}

				iVar6 = iVar6 + 1;
				pOpen = pOpen + 1;
			} while (iVar6 < 4);

			return false;
		}

#ifdef PLATFORM_PS2
		_edFile_MakeSceOpenFlag(&sceOpenFlags, pFile->openFlags);
#endif

		pcVar4 = edFilePathGetFilePath(szPath);
		edStrCopy(local_220, pcVar4 + 1);
		pcVar4 = local_220;

		while (*pcVar4 != '\0') {
			if (*pcVar4 == '\\') {
				*pcVar4 = '/';
			}

			pcVar4 = pcVar4 + 1;
		}

		syncResult = 0;

#ifdef PLATFORM_PS2
		iVar5 = sceMcOpen(pSpaceStruct->port, pSpaceStruct->slot, local_220, sceOpenFlags);
		syncResult = -1;
		if (iVar5 < 0) {
			// Failed to open, work out the error.
			MCardSceXxxDecodeValue(iVar5);
			bVar2 = false;
		}
		else {
			iVar5 = sceMcSync(0, (int*)0x0, &local_10);
			if (iVar5 == 1) {
				syncResult = local_10;
				if (local_10 < 0) {
					MCardSyncDecodeValue(local_10);
					bVar2 = false;
				}
				else {
					bVar2 = true;
				}
			}
			else {
				bVar2 = false;
			}
		}
#else
		bVar2 = OpenFileWin(pFile, local_220, pOpenFile);
#endif

		if (!bVar2) {
			iVar6 = 0;
			MC_OpenFile* pOpen = this->field_0x21c;
			do {
				if (pOpen == pOpenFile) {
					this->field_0x218[iVar6] = 0;
					memset(this->field_0x21c + iVar6, 0, sizeof(MC_OpenFile));
					return false;
				}

				iVar6 = iVar6 + 1;
				pOpen = pOpen + 1;
			} while (iVar6 < 4);

			return false;
		}

#ifdef PLATFORM_PS2
		pOpenFile->syncResult = syncResult;
#endif
		pOpenFile->pSpaceStruct = pSpaceStruct;
		pFile->pFileData = pOpenFile;

		if ((pFile->openFlags & 6) == 0) {
#ifdef PLATFORM_PS2
			iVar5 = sceMcGetDir(pSpaceStruct->port, pSpaceStruct->slot, local_220, 0, 1, &sStack1664);
			if (iVar5 < 0) {
				MCardSceXxxDecodeValue(iVar5);
				bVar2 = false;
			}
			else {
				iVar5 = sceMcSync(0, (int*)0x0, &local_14);
				if (iVar5 == 1) {
					if (local_14 < 0) {
						MCardSyncDecodeValue(local_14);
						bVar2 = false;
					}
					else {
						bVar2 = true;
					}
				}
				else {
					bVar2 = false;
				}
			}
			if (!bVar2) {
				iVar6 = 0;
				MC_OpenFile* pOpen = this->field_0x21c;
				do {
					if (pOpen == pOpenFile) {
						this->field_0x218[iVar6] = 0;
						memset(this->field_0x21c + iVar6, 0, sizeof(MC_OpenFile));
						return false;
					}

					iVar6 = iVar6 + 1;
					pOpen = pOpen + 1;
				} while (iVar6 < 4);

				return false;
			}

			pFileInfo = GetFileInfoFromFile(pFile);
			_MCardTranslateFileInfo(pFileInfo, &sStack1664);
#else
			pFileInfo = GetFileInfoFromFile(pFile);
			WinTranslateFileInfo(pFileInfo, szPath);
#endif
		}

		return true;
	}

	if (pFile->nbQueuedActions == 0) {
		iVar5 = 0;
		do {
			if (this->field_0x218[iVar5] == 0) {
				this->field_0x218[iVar5] = 1;
				pOpenFile = this->field_0x21c + iVar5;
				memset(pOpenFile, 0, sizeof(MC_OpenFile));
				goto LAB_0025f4b8;
			}

			iVar5 = iVar5 + 1;
		} while (iVar5 < 4);

		pOpenFile = (MC_OpenFile*)0x0;

	LAB_0025f4b8:
		if (pOpenFile == (MC_OpenFile*)0x0) {
			return false;
		}

		edFilePathSplit((char*)0x0, local_420, (char*)0x0, (char*)0x0, szPath);

		if (local_420[0] + -0x30 < 2) {
			pSpaceStruct = this->aSpaceStructs + (local_420[0] + -0x30);
		}
		else {
			pSpaceStruct = (MCSpaceStruct_0x10*)0x0;
		}

		if (pSpaceStruct == (MCSpaceStruct_0x10*)0x0) {
			iVar6 = 0;
			MC_OpenFile* pOpen = this->field_0x21c;
			do {
				if (pOpen == pOpenFile) {
					this->field_0x218[iVar6] = 0;
					memset(this->field_0x21c + iVar6, 0, sizeof(MC_OpenFile));
					return false;
				}

				iVar6 = iVar6 + 1;
				pOpen = pOpen + 1;
			} while (iVar6 < 4);

			return false;
		}

		char* pPath = edFilePathGetFilePath(szPath);
		edStrCopy((pFile->fileInfo).name, pPath + 1);
		pPath = (pFile->fileInfo).name;

		while (*pPath != '\0') {
			if (*pPath == '\\') {
				*pPath = '/';
			}
			pPath = pPath + 1;
		}

		pOpenFile->pSpaceStruct = pSpaceStruct;
		pFile->pFileData = pOpenFile;
		getnowaitfilestack()->AddFileOpen(pFile, (pFile->fileInfo).name);
	}
	else {
#ifdef PLATFORM_PS2
		IMPLEMENTATION_GUARD(
		_edFile_MakeSceOpenFlag(&sceOpenFlags, pFile->openFlags);
		MCSpaceStruct_0x10* pOpenFile = reinterpret_cast<MC_OpenFile*>(pFile->pFileData)->pSpaceStruct;
		sceMcOpen(pOpenFile->port, pOpenFile->slot, szPath, sceOpenFlags);
		)
#else
		OpenFileWin(pFile, szPath, reinterpret_cast<MC_OpenFile*>(pFile->pFileData));
#endif
	}

	return true;
}

#ifdef PLATFORM_WIN
bool WinFileClose(MC_OpenFile* pOpenFile)
{
	if (pOpenFile->pStream) {
		// Close the file stream
		std::ifstream* pIfStream = dynamic_cast<std::ifstream*>(static_cast<std::istream*>(pOpenFile->pStream));
		if (pIfStream) {
			if (pIfStream->is_open()) {
				pIfStream->close();
			}
			delete pIfStream;
			pOpenFile->pStream = nullptr;
			return true;
		}
		std::ofstream* pOfStream = dynamic_cast<std::ofstream*>(static_cast<std::ostream*>(pOpenFile->pStream));
		if (pOfStream) {
			if (pOfStream->is_open()) {
				pOfStream->close();
			}
			delete pOfStream;
			pOpenFile->pStream = nullptr;
			return true;
		}
		std::fstream* pFStream = dynamic_cast<std::fstream*>(static_cast<std::iostream*>(pOpenFile->pStream));
		if (pFStream) {
			if (pFStream->is_open()) {
				pFStream->close();
			}
			delete pFStream;
			pOpenFile->pStream = nullptr;
			return true;
		}
		// Unknown stream type
		return false;
	}
	return true; // Nothing to close
}
#endif

bool edCFiler_MemoryCard::close(edFILEH* pFile)
{
	bool bSuccess;
	MC_OpenFile* pOpenFile;
	int iVar1;

	pOpenFile = (MC_OpenFile*)GetInternalData_0025b2e0(pFile);
	if (pOpenFile == (MC_OpenFile*)0x0) {
		bSuccess = false;
	}
	else {
#ifdef PLATFORM_PS2
		iVar1 = sceMcClose(pOpenFile->syncResult);
		if (iVar1 < 0) {
			MCardSceXxxDecodeValue(iVar1);
			bSuccess = false;
		}
		else {
			bSuccess = true;
		}
#else
		bSuccess = WinFileClose(pOpenFile);
#endif
	}
	return bSuccess;
}

#ifdef PLATFORM_WIN
static bool WinFileRead(MC_OpenFile* pOpenFile, void* pDst, uint requiredSize)
{
	if (pOpenFile->pStream) {
		std::ifstream* pIfStream = dynamic_cast<std::ifstream*>(static_cast<std::istream*>(pOpenFile->pStream));
		if (pIfStream) {
			if (pIfStream->is_open()) {
				pIfStream->read(static_cast<char*>(pDst), requiredSize);
				return pIfStream->good();
			}
			return false;
		}
		std::fstream* pFStream = dynamic_cast<std::fstream*>(static_cast<std::iostream*>(pOpenFile->pStream));
		if (pFStream) {
			if (pFStream->is_open()) {
				pFStream->read(static_cast<char*>(pDst), requiredSize);
				return pFStream->good();
			}
			return false;
		}

		// Not a readable stream
		return false;
	}

	return false; // No stream to read from
}
#endif

uint edCFiler_MemoryCard::read(edFILEH* pFile, void* pDst, uint requiredSize)
{
	bool bSuccess;
	MC_OpenFile* pOpenFile;
	int iVar1;

	pOpenFile = (MC_OpenFile*)GetInternalData_0025b2e0(pFile);
	if (pOpenFile == (MC_OpenFile*)0x0) {
		bSuccess = false;
	}
	else {
#ifdef PLATFORM_PS2
		iVar1 = sceMcRead(pOpenFile->syncResult, pDst, requiredSize);
		if (iVar1 < 0) {
			MCardSceXxxDecodeValue(iVar1);
			bSuccess = false;
		}
		else {
			bSuccess = true;
		}
#else
		bSuccess = WinFileRead(pOpenFile, pDst, requiredSize);
#endif
	}

	return bSuccess;
}

#ifdef PLATFORM_WIN
bool WinFileWrite(MC_OpenFile* pOpenFile, void* pDst, uint size)
{
	if (pOpenFile->pStream) {
		std::ofstream* pOfStream = dynamic_cast<std::ofstream*>(static_cast<std::ostream*>(pOpenFile->pStream));
		if (pOfStream) {
			if (pOfStream->is_open()) {
				pOfStream->write(static_cast<const char*>(pDst), size);
				return pOfStream->good();
			}
			return false;
		}

		std::fstream* pFStream = dynamic_cast<std::fstream*>(static_cast<std::iostream*>(pOpenFile->pStream));
		if (pFStream) {
			if (pFStream->is_open()) {
				pFStream->write(static_cast<const char*>(pDst), size);
				return pFStream->good();
			}
			return false;
		}

		// Not a writable stream
		return false;
	}

	return false; // No stream to write to
}
#endif

bool edCFiler_MemoryCard::write(edFILEH* pFile, void* pDst, uint size)
{
	MCSpaceStruct_0x10* pSpaceStruct;
	bool bSuccess;
	MC_OpenFile* pOpenFile;
	int iVar2;

	pOpenFile = (MC_OpenFile*)GetInternalData_0025b2e0(pFile);
	if (pOpenFile == (MC_OpenFile*)0x0) {
		bSuccess = false;
	}
	else {
		pSpaceStruct = pOpenFile->pSpaceStruct;
		pSpaceStruct->field_0x0 = 0;
		pSpaceStruct->bIsSyncIdle = 1;

#ifdef PLATFORM_PS2
		iVar2 = sceMcWrite(pOpenFile->syncResult, pDst, size);
		if (iVar2 < 0) {
			MCardSceXxxDecodeValue(iVar2);
			bSuccess = false;
		}
		else {
			bSuccess = true;
		}
#else
		WinFileWrite(pOpenFile, pDst, size);
#endif
	}

	return bSuccess;
}

bool edCFiler_MemoryCard::seek(edFILEH* pFile)
{
	IMPLEMENTATION_GUARD();
}

#ifdef PLATFORM_WIN
static bool DeleteFileWin(char* szPath)
{
	try {
		const std::filesystem::path pathToDelete(szPath);
		if (std::filesystem::remove(pathToDelete)) {
			return true;
		}
		else {
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error deleting file: " << e.what() << std::endl;
		return false;
	}
}
#endif

bool edCFiler_MemoryCard::erase(char* szPath)
{
	bool bSuccess;
	char* pcVar1;
	int iVar2;
	MCSpaceStruct_0x10* pcVar4;
	char local_410[512];
	char local_210[524];
	int local_4;

	edFilePathSplit((char*)0x0, local_410, (char*)0x0, (char*)0x0, szPath);

	if (local_410[0] + -0x30 < 2) {
		pcVar4 = this->aSpaceStructs + (local_410[0] + -0x30);
	}
	else {
		pcVar4 = (MCSpaceStruct_0x10*)0x0;
	}

	if (pcVar4->field_0x0 == 0) {
		this->field_0x23c.AddFilerSync(this);
	}

	edFileNoWaitStackFlush(this);

	if (pcVar4->field_0x1 == 0) {
		pcVar4 = (MCSpaceStruct_0x10*)0x0;
	}
	if (pcVar4 == (MCSpaceStruct_0x10*)0x0) {
		bSuccess = false;
	}
	else {
		pcVar4->field_0x0 = 0;
		pcVar4->bIsSyncIdle = 1;
		pcVar1 = edFilePathGetFilePath(szPath);
		edStrCopy(local_210, pcVar1 + 1);
		pcVar1 = local_210;
		while (*pcVar1 != '\0') {
			if (*pcVar1 == '\\') {
				*pcVar1 = '/';
			}

			pcVar1 = pcVar1 + 1;
		}

#ifdef PLATFORM_PS2
		iVar2 = sceMcDelete(pcVar4->port, pcVar4->slot, local_210);
		if (iVar2 < 0) {
			MCardSceXxxDecodeValue(iVar2);
			bSuccess = false;
		}
		else {
			iVar2 = sceMcSync(0, (int*)0x0, &local_4);
			if (iVar2 == 1) {
				if (local_4 < 0) {
					MCardSyncDecodeValue(local_4);
					bSuccess = false;
				}
				else {
					bSuccess = true;
				}
			}
			else {
				bSuccess = false;
			}
		}
#else
		DeleteFileWin(local_210);
#endif
	}

	return bSuccess;
}

bool edCFiler_MemoryCard::waitcmdend(/*missing args*/)
{
#ifdef PLATFORM_PS2
	int iVar1;

	iVar1 = sceMcGetInfo(this->aSpaceStructs[this->activeMemCardIndex].port, this->aSpaceStructs[this->activeMemCardIndex].slot, &this->infoType, &this->infoFree, &this->infoFormat);
	if (-1 >= iVar1) {
		MCardSceXxxDecodeValue(iVar1);
	}

	return -1 < iVar1;
#else
	return true;
#endif
}

bool edCFiler_MemoryCard::findfile(edCFileFind* pFileFind, int mode)
{
	bool bResult;
	char* pcVar1;
	int iVar2;
	MCSpaceStruct_0x10* pSlotInfo;
	int mcMode;
#ifdef PLATFORM_PS2
	sceMcTblGetDir tblGetDirInfo;
#endif
	char szSplitPath[512];
	char szDirPath[520];
	int local_8;
	int local_4;

#ifdef PLATFORM_PS2
	mcMode = SCE_RDONLY;
#endif // PLATFORM_PS2

	if (mode == 2) {
		pFileFind->field_0x8 = 0;
		bResult = true;
	}
	else {
		if (mode != 1) {
			if (mode != 0) {
				return false;
			}

			edStrCopy(pFileFind->szSlotPathB, pFileFind->szSlotPathA);
			pFileFind->field_0x8 = 0;
			mcMode = 0;
		}

		edFilePathSplit((char*)0x0, szSplitPath, (char*)0x0, (char*)0x0, pFileFind->szSlotPathB);

		if (szSplitPath[0] + -0x30 < 2) {
			pSlotInfo = this->aSpaceStructs + (szSplitPath[0] + -0x30);
		}
		else {
			pSlotInfo = (MCSpaceStruct_0x10*)0x0;
		}

		if (pSlotInfo->field_0x0 == 0) {
			this->field_0x23c.AddFilerSync(this);
		}

		edFileNoWaitStackFlush(this);

		if (pSlotInfo->field_0x1 == 0) {
			pSlotInfo = (MCSpaceStruct_0x10*)0x0;
		}

		if (pSlotInfo == (MCSpaceStruct_0x10*)0x0) {
			bResult = false;
		}
		else {
			pcVar1 = edFilePathGetFilePath(pFileFind->szSlotPathB);
			edStrCopy(szDirPath, pcVar1 + 1);
			pcVar1 = szDirPath;
			while (*pcVar1 != '\0') {
				if (*pcVar1 == '\\') {
					*pcVar1 = '/';
				}

				pcVar1 = pcVar1 + 1;
			}

#ifdef PLATFORM_WIN
			pFileFind->field_0x8 = pFileFind->field_0x8 + 1;
			return WinTranslateFileInfo(pFileFind->pFileInfo, szDirPath);
#else
			iVar2 = sceMcGetDir(pSlotInfo->port, pSlotInfo->slot, szDirPath, mcMode, 1, &tblGetDirInfo);
			local_4 = -1;
			if (iVar2 < 0) {
				MCardSceXxxDecodeValue(iVar2);
				bResult = false;
			}
			else {
				iVar2 = sceMcSync(0, (int*)0x0, &local_8);
				if (iVar2 == 1) {
					local_4 = local_8;
					if (local_8 < 0) {
						MCardSyncDecodeValue(local_8);
						bResult = false;
					}
					else {
						bResult = true;
					}
				}
				else {
					bResult = false;
				}
			}

			if (bResult) {
				if (local_4 == 0) {
					bResult = false;
				}
				else {
					pFileFind->field_0x8 = pFileFind->field_0x8 + 1;
					_MCardTranslateFileInfo(pFileFind->pFileInfo, &tblGetDirInfo);
					bResult = true;
				}
			}
			else {
				bResult = false;
			}
#endif // !PLATFORM_WIN
		}
	}
	return bResult;
}

bool edCFiler_MemoryCard::isnowaitcmdend(edFILE_STACK_ELEMENT* pElement)
{
	bool bResult;
	MC_OpenFile* pOpenFile;
	int iVar2;
	int iVar3;
	MCSpaceStruct_0x10* pSlotInfo;
	int outResult;
	int local_4;
	edFILEH* pFile;

	pFile = reinterpret_cast<edFILEH*>(pElement->pData);

	pOpenFile = (MC_OpenFile*)GetInternalData_0025b2e0(pFile);
	if (pOpenFile == (MC_OpenFile*)0x0) {
		return false;
	}

	switch (pElement->action) {
	case EBankAction::LOAD:
		bResult = true;
		break;
	case EBankAction::SEEK:
	case EBankAction::READ:
	case EBankAction::WRITE:
	case EBankAction::CLOSE:
	case EBankAction::OPEN:
#ifdef PLATFORM_PS2
		const int syncResult = sceMcSync(1, (int*)0x0, &local_4);
		if (syncResult != sceMcExecIdle) {
			if (syncResult == sceMcExecFinish) {
				if (local_4 < 0) {
					MCardSyncDecodeValue();
					pFile->bInUse = false;
				}

				iVar2 = 0;
				if (pElement->action == EBankAction::CLOSE) {
					MC_OpenFile* pOpen = this->field_0x21c;
					do {
						if (pOpen == pOpenFile) {
							this->field_0x218[iVar2] = 0;
							memset(this->field_0x21c + iVar2, 0, sizeof(MC_OpenFile));

							return true;
						}

						iVar2 = iVar2 + 1;
						pOpen = pOpen + 1;
					} while (iVar2 < 4);
				}
				else {
					if (pElement->action == EBankAction::OPEN) {
						pOpenFile->syncResult = local_4;
					}
				}
				return true;
			}

			if (syncResult == sceMcExecRun) {
				return false;
			}
		}
		bResult = false;
#else
		iVar2 = 0;
		if (pElement->action == EBankAction::CLOSE) {
			MC_OpenFile* pOpen = this->field_0x21c;
			do {
				if (pOpen == pOpenFile) {
					this->field_0x218[iVar2] = 0;
					memset(this->field_0x21c + iVar2, 0, sizeof(MC_OpenFile));

					return true;
				}

				iVar2 = iVar2 + 1;
				pOpen = pOpen + 1;
			} while (iVar2 < 4);
		}
		return true;
#endif
		break;
	case EBankAction::SYNC:
#ifdef PLATFORM_PS2
		const int syncResult = sceMcSync(1, (int*)0x0, &outResult);
		if (syncResult != sceMcExecIdle) {
			if (syncResult == sceMcExecFinish) {
				pSlotInfo = this->aSpaceStructs + this->activeMemCardIndex;
				if (outResult == 0) {
					if (((pSlotInfo->field_0x1 != 0) && (this->infoType == 2)) && (this->infoFormat != 0)) {
						pSlotInfo->free = this->infoFree;
					}

					goto LAB_0025ea40;
				}

				pSlotInfo->field_0x0 = 1;
				if (outResult == sceMcResDeniedPermit) {
				LAB_0025e958:
					if (pSlotInfo->field_0x1 != 0) {
						pSlotInfo->bIsSyncIdle = 1;
						pSlotInfo->field_0x1 = 0;
					}

					edSysHandlersCall(edFileHandlers.mainIdentifier, edFileHandlers.entries, edFileHandlers.maxEventID, 8, (void*)0x0);
				}
				else {
					if (outResult == sceMcResNoFormat) {
						pSlotInfo->field_0x1 = 1;
						pSlotInfo->bIsSyncIdle = 1;
						edSysHandlersCall(edFileHandlers.mainIdentifier, edFileHandlers.entries, edFileHandlers.maxEventID, 10, (void*)0x0);
					}
					else {
						if (outResult != sceMcResChangedCard) goto LAB_0025e958;
						pSlotInfo->field_0x1 = 1;
						pSlotInfo->bIsSyncIdle = 1;
						edSysHandlersCall(edFileHandlers.mainIdentifier, edFileHandlers.entries, edFileHandlers.maxEventID, 9, (void*)0x0);
					}
				}

				pSlotInfo->field_0x3 = 0;
				pSlotInfo->free = 0;
				if (((pSlotInfo->field_0x1 != 0) && (this->infoType == 2)) && (this->infoFormat != 0)) {
					pSlotInfo->field_0x3 = 1;
					pSlotInfo->free = this->infoFree;
				}

			LAB_0025ea40:
				this->activeMemCardIndex = this->activeMemCardIndex + 1;
				if (this->activeMemCardIndex == 2) {
					this->activeMemCardIndex = 0;
				}

				return true;
			}

			if (syncResult == sceMcExecRun) {
				return false;
			}
		}
#else
		pSlotInfo = this->aSpaceStructs + this->activeMemCardIndex;
		try {
			std::filesystem::space_info si = std::filesystem::space(std::filesystem::current_path());
			pSlotInfo->free = si.available;
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Error: " << e.what() << "\n";
		}

		this->activeMemCardIndex = this->activeMemCardIndex + 1;
		if (this->activeMemCardIndex == 2) {
			this->activeMemCardIndex = 0;
		}

		pSlotInfo->field_0x0 = 1;
		pSlotInfo->field_0x1 = 1;
		pSlotInfo->field_0x3 = 1;

		return true;
#endif

		bResult = false;
		break;
	default:
		bResult = true;
	}

	return bResult;
}

#ifdef PLATFORM_WIN
static bool MakeDirWin(char* szPath)
{
	// Use std C++ filesystem to create the directory
	try {
		std::filesystem::path dirPath(szPath);
		return std::filesystem::create_directories(dirPath);
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating directory: " << e.what() << std::endl;
		return false;
	}
}
#endif // PLATFORM_WIN

bool edCFiler_MemoryCard::mkdir(char* szPath)
{
	bool bSuccess;
	char* pcVar1;
	int iVar2;
	MCSpaceStruct_0x10* pSlotInfo;
	char local_410[512];
	char cStack528[524];
	int local_4;

	edFilePathSplit((char*)0x0, local_410, (char*)0x0, (char*)0x0, szPath);

	if (local_410[0] + -0x30 < 2) {
		pSlotInfo = this->aSpaceStructs + (local_410[0] + -0x30);
	}
	else {
		pSlotInfo = (MCSpaceStruct_0x10*)0x0;
	}

	if (pSlotInfo->field_0x0 == 0) {
		this->field_0x23c.AddFilerSync(this);
	}

	edFileNoWaitStackFlush(this);

	if (pSlotInfo->field_0x1 == 0) {
		pSlotInfo = (MCSpaceStruct_0x10*)0x0;
	}

	if (pSlotInfo == (MCSpaceStruct_0x10*)0x0) {
		bSuccess = false;
	}
	else {
		pSlotInfo->field_0x0 = 0;
		pSlotInfo->bIsSyncIdle = 1;
		edFilePathSplit((char*)0x0, (char*)0x0, cStack528, (char*)0x0, szPath);
		pcVar1 = edStrChr(cStack528 + 1, '\\');
		if (pcVar1 != (char*)0x0) {
			*pcVar1 = '\0';
		}

#ifdef PLATFORM_PS2
		iVar2 = sceMcMkdir(pSlotInfo->port, pSlotInfo->slot, cStack528+ 1);
		if (iVar2 < 0) {
			MCardSceXxxDecodeValue(iVar2);
			bSuccess = false;
		}
		else {
			iVar2 = sceMcSync(0, (int*)0x0, &local_4);
			if (iVar2 == 1) {
				if (local_4 < 0) {
					MCardSyncDecodeValue(local_4);
					bSuccess = false;
				}
				else {
					bSuccess = true;
				}
			}
			else {
				bSuccess = false;
			}
		}
#else
		bSuccess = MakeDirWin(cStack528 + 1);
#endif // PLATFORM_PS2
	}

	return bSuccess;
}

bool edCFiler_MemoryCard::setattr(/*missing args*/)
{
	IMPLEMENTATION_GUARD();
}

bool edCFiler_MemoryCard::getfreespace(char* szPath, ulong* pFreeBytes, ulong* pFreeClusters, byte* param_5)
{
	bool bResult;
	MCSpaceStruct_0x10* pSlotInfo;
	char local_200[512];

	edFilePathSplit((char*)0x0, local_200, (char*)0x0, (char*)0x0, szPath);

	if (local_200[0] + -0x30 < 2) {
		pSlotInfo = this->aSpaceStructs + (local_200[0] - 0x30);
	}
	else {
		pSlotInfo = (MCSpaceStruct_0x10*)0x0;
	}

	if (pSlotInfo == (MCSpaceStruct_0x10*)0x0) {
		if (pFreeClusters != (ulong*)0x0) {
			*pFreeClusters = 0;
		}

		if (pFreeBytes != (ulong*)0x0) {
			*pFreeBytes = 0;
		}

		if (param_5 != (byte*)0x0) {
			*param_5 = 0;
		}

		bResult = false;
	}
	else {
		if (pSlotInfo->field_0x1 == 0) {
			if (pFreeClusters != (ulong*)0x0) {
				*pFreeClusters = 0;
			}

			if (pFreeBytes != (ulong*)0x0) {
				*pFreeBytes = 0;
			}

			if (param_5 != (byte*)0x0) {
				*param_5 = 0;
			}

			bResult = false;
		}
		else {
			if (pFreeClusters != (ulong*)0x0) {
				*pFreeClusters = (ulong)pSlotInfo->free;
			}

			if (pFreeBytes != (ulong*)0x0) {
				*pFreeBytes = (ulong)pSlotInfo->free << 10;
			}

			if (param_5 != (byte*)0x0) {
				*param_5 = pSlotInfo->field_0x3;
			}

			bResult = true;
		}
	}

	return bResult;
}

bool edCFiler_MemoryCard::isidle(char* szPath, int param_3)
{
	uint freeBytes;
	MCSpaceStruct_0x10* pCurEntry;
	char buff[512];

	edFilePathSplit((char*)0x0, buff, (char*)0x0, (char*)0x0, szPath);

	if (buff[0] + -0x30 < 2) {
		pCurEntry = this->aSpaceStructs + (buff[0] - 0x30);
	}
	else {
		pCurEntry = (MCSpaceStruct_0x10*)0x0;
	}

	freeBytes = 0;

	if ((pCurEntry != (MCSpaceStruct_0x10*)0x0) && (freeBytes = (uint)pCurEntry->bIsSyncIdle, param_3 != 0)) {
		pCurEntry->bIsSyncIdle = 0;
	}

	return freeBytes;
}

bool edCFiler_MemoryCard::format(/*missing args*/)
{
	IMPLEMENTATION_GUARD();
}

MCSpaceStruct_0x10::MCSpaceStruct_0x10()
{
	this->field_0x0 = 0;

	return;
}

void _edFileMCardAddFiler(void)
{
	edFilerList.add_filer(&edFiler_MemoryCard);

	return;
}