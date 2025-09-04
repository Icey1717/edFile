#include "edFileNoWaitStack.h"
#include "edFile.h"
#include "edFileFiler.h"

edCFileNoWaitStack edCFiler_28_ARRAY_004697a0;

edCFileNoWaitStack::edCFileNoWaitStack()
	: nbInUse(0)
	, currentIndex(0)
	, nextAction(EBankAction::LOAD)
{

}

edFILE_STACK_ELEMENT* edCFileNoWaitStack::AddFiler(edCFiler* pFiler, EBankAction action)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = (edFILE_STACK_ELEMENT*)0x0;

	if (this->nbInUse < 0x18) {
		pElement = &this->aElements[(this->currentIndex + this->nbInUse) % 0x18];
		pElement->action = action;

		if (this->nbInUse == 0) {
			this->nextAction = this->aElements[this->currentIndex].action;
		}

		this->nbInUse = this->nbInUse + 1;

	}
	if (pElement == (edFILE_STACK_ELEMENT*)0x0) {
		pElement = (edFILE_STACK_ELEMENT*)0x0;
	}
	else {
		pElement->bIsFiler = 1;
		pElement->pData = pFiler;
	}

	return pElement;
}

edFILE_STACK_ELEMENT* edCFileNoWaitStack::AddFile(edFILEH* pFile, EBankAction action)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = (edFILE_STACK_ELEMENT*)0x0;

	if (this->nbInUse < 0x18) {
		pElement = &this->aElements[(this->currentIndex + this->nbInUse) % 0x18];
		pElement->action = action;

		if (this->nbInUse == 0) {
			this->nextAction = this->aElements[this->currentIndex].action;
		}

		this->nbInUse = this->nbInUse + 1;
	}

	if (pElement == (edFILE_STACK_ELEMENT*)0x0) {
		pElement = (edFILE_STACK_ELEMENT*)0x0;
	}
	else {
		pElement->bIsFiler = 0;
		pElement->pData = pFile;
		pFile->nbQueuedActions = pFile->nbQueuedActions + 1;
	}

	return pElement;
}

bool edCFileNoWaitStack::AddFilerSync(edCFiler* pFiler)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFiler(pFiler, EBankAction::SYNC);

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

bool edCFileNoWaitStack::AddFileClose(edFILEH* pFile)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFile(pFile, EBankAction::CLOSE);

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

bool edCFileNoWaitStack::AddFileWrite(edFILEH* pFile, void* pData, uint nbWriteBytes)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFile(pFile, EBankAction::WRITE);

	if (pElement != (edFILE_STACK_ELEMENT*)0x0) {
		pElement->pWriteDst = pData;
		pElement->nbWriteBytes = nbWriteBytes;
	}

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

bool edCFileNoWaitStack::AddFileRead(edFILEH* pFile, void* pData, uint nbReadBytes)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFile(pFile, EBankAction::READ);

	if (pElement != (edFILE_STACK_ELEMENT*)0x0) {
		pElement->pReadBuffer = pData;
		pElement->nbReadBytes = nbReadBytes;
	}

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

bool edCFileNoWaitStack::AddFileSeek(edFILEH* pFile, uint seekOffset)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFile(pFile, EBankAction::SEEK);

	if (pElement != (edFILE_STACK_ELEMENT*)0x0) {
		pElement->seekOffset = seekOffset;
	}

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

bool edCFileNoWaitStack::AddFileOpen(edFILEH* pFile, char* szPath)
{
	edFILE_STACK_ELEMENT* pElement;

	pElement = AddFile(pFile, EBankAction::OPEN);

	if (pElement != (edFILE_STACK_ELEMENT*)0x0) {
		pElement->szPath = szPath;
	}

	return pElement != (edFILE_STACK_ELEMENT*)0x0;
}

// #Hack
char* g_FileActionNames[] = {
	"WRITE",
	"SYNC",
	"CLOSE",
	"LOAD",
	"OPEN",
	"READ",
	"SEEK"
};

void edFileNoWaitStackCallBack(edCFileNoWaitStack* pStack)
{
	EBankAction action;
	edCFiler* pFiler;
	edFILE_STACK_ELEMENT* pElement;
	int nbInUse;
	ulong bResult;
	edFILEH* pFile;
	edFILE_STACK_ELEMENT* pNextElement;

	if (pStack->nbInUse != 0) {
		action = pStack->nextAction;

		pElement = pStack->aElements + pStack->currentIndex;
		if (action == EBankAction::LOAD) {
			const int filerMode = pElement->bIsFiler;
			pFile = (edFILEH*)0x0;
			pFiler = (edCFiler*)0x0;

			if (filerMode == 0) {
				pFile = reinterpret_cast<edFILEH*>(pElement->pData);
				pFiler = pFile->pOwningFiler;
			}
			else {
				if (filerMode == 1) {
					pFiler = reinterpret_cast<edCFiler*>(pElement->pData);
				}
			}

			if (pFiler->isnowaitcmdend(pElement) != false) {
				if (pElement->bIsFiler == 0) {
					if (pFile->bInUse == 0) {
						nbInUse = pStack->nbInUse;
						while (nbInUse != 0) {
							pFile = reinterpret_cast<edFILEH*>(pStack->aElements[pStack->currentIndex].pData);
							pFile->nbQueuedActions = pFile->nbQueuedActions + -1;
							edFileCmdBreak(pFile);
							pStack->nbInUse = pStack->nbInUse + -1;
							pStack->currentIndex = pStack->currentIndex + 1;
							if (pStack->currentIndex == 0x18) {
								pStack->currentIndex = 0;
							}

							nbInUse = pStack->nbInUse;
						}

						return;
					}

					pFile->nbQueuedActions = pFile->nbQueuedActions + -1;
				}

				if (true) {
					MY_LOG("Doing file action {}\n", g_FileActionNames[(int)pElement->action]);
					switch (pElement->action) {
					case EBankAction::SEEK:
						edSysHandlersCall(edFileHandlers.mainIdentifier,
							edFileHandlers.entries,
							edFileHandlers.maxEventID, ED_HANDLER_FILE_SEEK, (void*)0x0);
						break;
					case EBankAction::READ:
						edSysHandlersCall(edFileHandlers.mainIdentifier,
							edFileHandlers.entries,
							edFileHandlers.maxEventID, ED_HANDLER_FILE_READ, pElement->pReadBuffer);
						break;
					case EBankAction::WRITE:
						edSysHandlersCall(edFileHandlers.mainIdentifier,
							edFileHandlers.entries,
							edFileHandlers.maxEventID, ED_HANDLER_FILE_WRITE, pElement->pWriteDst);
						break;
					case EBankAction::CLOSE:
						edSysHandlersCall(edFileHandlers.mainIdentifier, edFileHandlers.entries, 
							edFileHandlers.maxEventID, ED_HANDLER_FILE_OPEN, (void*)0x0);

						if ((pFile->openFlags & 0x20) == 0) {
							int i = 0;
							do {
								if (&edFileHandleData[i] == pFile) {
									edFileHandleTable[i] = 0;
									memset(&edFileHandleData[i], 0, sizeof(edFILEH));
									break;
								}

								i = i + 1;
							} while (i < 0x10);
						}
					}
				}

				pStack->nbInUse = pStack->nbInUse + -1;

				if (pStack->nbInUse != 0) {
					pStack->currentIndex = pStack->currentIndex + 1;

					if (pStack->currentIndex == 0x18) {
						pStack->currentIndex = 0;
						pNextElement = pStack->aElements;
					}
					else {
						pNextElement = pElement + 1;
					}

					pStack->nextAction = pNextElement->action;
				}
			}
		}
		else {
			const int filerMode = pElement->bIsFiler;

			if (filerMode == 1) {
				if (action == SYNC) {
					edCFiler* pFiler = reinterpret_cast<edCFiler*>(pElement->pData);
					bResult = pFiler->waitcmdend();
				}
				else {
					bResult = 0;
				}
			}
			else {
				if (filerMode == 0) {
					pFile = reinterpret_cast<edFILEH*>(pElement->pData);
					pFiler = pFile->pOwningFiler;
					pFile->action = action;
					action = pStack->nextAction;

					MY_LOG("Doing file action {}\n", g_FileActionNames[(int)action]);

					if (action == CLOSE) {
						bResult = pFiler->close(pFile);
					}
					else {
						if (action == EBankAction::WRITE) {							
							bResult = pFiler->write(pFile, pElement->pWriteDst, pElement->nbWriteBytes);
							if (bResult != 0) {
								pFile->seekOffset = pFile->seekOffset + pElement->nbWriteBytes;
							}
						}
						else {
							if (action == EBankAction::READ) {
								bResult = pFiler->read(pFile, pElement->pReadBuffer, pElement->nbReadBytes);
								if (bResult != 0) {
									pFile->seekOffset = pFile->seekOffset + pElement->nbReadBytes;
								}
							}
							else {
								if (action == EBankAction::SEEK) {
									pFile->seekOffset = pElement->seekOffset;
									bResult = pFiler->seek(pFile);
								}
								else {
									if (action == EBankAction::OPEN) {
										bResult = pFiler->open(pFile, pElement->szPath);
									}
									else {
										bResult = 0;
									}
								}
							}
						}
					}

					if (bResult != 0) {
						pFile->bInUse = 1;
					}
				}
			}

			if (bResult == 0) {
				nbInUse = pStack->nbInUse;

				while (nbInUse != 0) {
					pFile = reinterpret_cast<edFILEH*>(pStack->aElements[pStack->currentIndex].pData);
					pFile->nbQueuedActions = pFile->nbQueuedActions + -1;
					edFileCmdBreak(pFile);
					pStack->nbInUse = pStack->nbInUse + -1;
					pStack->currentIndex = pStack->currentIndex + 1;
					if (pStack->currentIndex == 0x18) {
						pStack->currentIndex = 0;
					}
					nbInUse = pStack->nbInUse;
				}
			}
			else {
				pStack->nextAction = EBankAction::LOAD;
			}
		}
	}

	return;
}

void edFileNoWaitStackFlush(void)
{
	edCFiler* pFiler;
	edCFileNoWaitStack* pStack;

	for (pFiler = edFilerList.get_root(); pFiler != (edCFiler*)0x0; pFiler = pFiler->pNextEd) {
		pStack = pFiler->getnowaitfilestack();
		if (pStack != (edCFileNoWaitStack*)0x0) {
			edFileNoWaitStackCallBack(pStack);
		}
	}
	return;
}

void edFileNoWaitStackFlush(edCFiler* pFiler)
{
	bool bFound;
	edCFiler* pCurFiler;
	edCFileNoWaitStack* pStack;

	do {
		bFound = true;

		for (pCurFiler = edFilerList.get_root(); pCurFiler != (edCFiler*)0x0; pCurFiler = pCurFiler->pNextEd) {
			pStack = pCurFiler->getnowaitfilestack();
			if ((pStack != (edCFileNoWaitStack*)0x0) && (pStack->nbInUse != 0)) {
				edFileNoWaitStackCallBack(pStack);

				if (pFiler == (edCFiler*)0x0) {
					bFound = false;
				}
				else {
					if (pCurFiler == pFiler) {
						bFound = false;
					}
				}
			}
		}
	} while (!bFound);

	return;
}