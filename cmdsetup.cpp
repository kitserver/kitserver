#include <stdio.h>
#include "imageutil.h"

#define DLL_PATH "kitserver\\kserv\0"

void printUsage(char* self)
{
	printf("Usage: %s <exefilename> [remove]\n", self);
}

void main(int argc, char** argv)
{
	if (argc < 2)
	{
		printUsage(argv[0]);
		return;
	}

	// get the address of LoadLibrary function, which is different 
	// on different machines (OS)
	DWORD loadLib = (DWORD)LoadLibrary;

	DWORD ep, ib, dVA, rdVA;
	DWORD loadLibAddr, kservAddr;
	DWORD newEntryPoint;

	FILE* f = fopen(argv[1], "r+b");
	if (f != NULL)
	{
		if (argc < 3)
		{
			// Install
			if (SeekEntryPoint(f))
			{
				fread(&ep, sizeof(DWORD), 1, f);
				printf("Entry point: %08x\n", ep);
			}
			if (SeekImageBase(f))
			{
				fread(&ib, sizeof(DWORD), 1, f);
				printf("Image base: %08x\n", ib);
			}
			if (SeekSectionVA(f, ".data"))
			{
				fread(&dVA, sizeof(DWORD), 1, f);
				//printf(".data virtual address: %08x\n", dVA);

				// just before .data starts - in the end of .rdata,
				// write the LoadLibrary address, and the name of kserv.dll
				BYTE buf[0x20], zero[0x20];
				ZeroMemory(zero, 0x20);
				ZeroMemory(buf, 0x20);

				fseek(f, dVA - 0x20, SEEK_SET);
				fread(&buf, 0x20, 1, f);
				if (memcmp(buf, zero, 0x20)==0)
				{
					// ok, we found an empty place. Let's live here.
					fseek(f, -0x20, SEEK_CUR);
					DWORD* p = (DWORD*)buf;
					p[0] = ep; // save old empty pointer for easy uninstall
					p[1] = loadLib;
					memcpy(buf + 8, DLL_PATH, lstrlen(DLL_PATH)+1);
					fwrite(buf, 0x20, 1, f);

					loadLibAddr = ib + dVA - 0x20 + sizeof(DWORD);
					printf("loadLibAddr = %08x\n", loadLibAddr);
					kservAddr = loadLibAddr + sizeof(DWORD);
					printf("kservAddr = %08x\n", kservAddr);
				}
				else
				{
					printf("Already installed.\n");
					fclose(f);
					return;
				}
			}
			if (SeekSectionVA(f, ".rdata"))
			{
				fread(&rdVA, sizeof(DWORD), 1, f);
				//printf(".rdata virtual address: %08x\n", rdVA);

				// just before .rdata starts, write the new entry point logic
				BYTE buf[0x20], zero[0x20];
				ZeroMemory(zero, 0x20);
				ZeroMemory(buf, 0x20);

				fseek(f, rdVA - 0x20, SEEK_SET);
				fread(&buf, 0x20, 1, f);
				if (memcmp(buf, zero, 0x20)==0)
				{
					// ok, we found an empty place. Let's live here.
					fseek(f, -0x20, SEEK_CUR);
					buf[0] = 0x68;  // push
					DWORD* p = (DWORD*)(buf + 1); p[0] = kservAddr;
					buf[5] = 0xff; buf[6] = 0x15; // call
					p = (DWORD*)(buf + 7); p[0] = loadLibAddr;
					buf[11] = 0xe9; // jmp
					p = (DWORD*)(buf + 12); p[0] = ib + ep - 5 - (ib + rdVA - 0x20 + 11);
					fwrite(buf, 0x20, 1, f);

					newEntryPoint = rdVA - 0x20;
				}
				else
				{
					printf("Already installed.\n");
					fclose(f);
					return;
				}
			}
			if (SeekEntryPoint(f))
			{
				// write new entry point
				fwrite(&newEntryPoint, sizeof(DWORD), 1, f);
				printf("New entry point: %08x\n", newEntryPoint);
			}
			if (SeekCodeSectionFlags(f))
			{
				DWORD flags;
				fread(&flags, sizeof(DWORD), 1, f);
				flags |= 0x80000000; // make code section writeable
				fseek(f, -sizeof(DWORD), SEEK_CUR);
				fwrite(&flags, sizeof(DWORD), 1, f);
			}
		}
		else if (lstrcmpi(argv[2], "remove")==0)
		{
			// Uninstall
			if (SeekEntryPoint(f))
			{
				fread(&ep, sizeof(DWORD), 1, f);
				printf("Current entry point: %08x\n", ep);
			}
			if (SeekSectionVA(f, ".data"))
			{
				fread(&dVA, sizeof(DWORD), 1, f);
				//printf(".data virtual address: %08x\n", dVA);

				// just before .data starts - in the end of .rdata,
				BYTE zero[0x20];
				ZeroMemory(zero, 0x20);
				fseek(f, dVA - 0x20, SEEK_SET);

				// read saved old entry point
				fread(&newEntryPoint, sizeof(DWORD), 1, f);
				if (newEntryPoint == 0)
				{
					printf("Already uninstalled.\n");
					fclose(f);
					return;
				}
				// zero out the bytes
				fseek(f, -sizeof(DWORD), SEEK_CUR);
				fwrite(zero, 0x20, 1, f);
			}
			if (SeekSectionVA(f, ".rdata"))
			{
				fread(&rdVA, sizeof(DWORD), 1, f);
				//printf(".rdata virtual address: %08x\n", rdVA);

				// just before .rdata starts, write the new entry point logic
				// zero out the bytes
				BYTE zero[0x20];
				ZeroMemory(zero, 0x20);
				fseek(f, rdVA - 0x20, SEEK_SET);
				fwrite(&zero, 0x20, 1, f);
			}
			if (SeekEntryPoint(f))
			{
				// write new entry point
				fwrite(&newEntryPoint, sizeof(DWORD), 1, f);
				printf("New entry point: %08x\n", newEntryPoint);
			}
			if (SeekCodeSectionFlags(f))
			{
				DWORD flags;
				fread(&flags, sizeof(DWORD), 1, f);
				flags &= 0x7fffffff; // make code section non-writeable again
				fseek(f, -sizeof(DWORD), SEEK_CUR);
				fwrite(&flags, sizeof(DWORD), 1, f);
			}
		}
		else
		{
			printUsage(argv[0]);
		}
		fclose(f);
	}
	else
	{
		printf("Error: unable to open %s for read/write\n", argv[1]);
		return;
	}
}
