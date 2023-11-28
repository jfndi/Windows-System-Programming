/*
 * Beep:
 * =====
 * This sample program illustrates the use NtOpenFile from user-mode.
 *
 * We wont use the Beep() Windows user-mode API but, instead, we will open a
 * handle to the Beep Device directly and, then, send it an ioctl to play the
 * sound.
 * Note that \Device\Beep is not a symbolic link, it is a device object. The
 * Beep driver does not create a corresponding symbolic link.
 */
#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <ntddbeep.h>

#pragma comment(lib, "ntdll")

int
main(int argc, const char* argv[])
{
	printf("beep [<frequency> <duration_in_msec>]\n");

	int freq = 800;
	int duration = 100;
	if (argc > 2)
	{
		freq = atoi(argv[1]);
		duration = atoi(argv[2]);
	}

	HANDLE hFile;
	OBJECT_ATTRIBUTES attr{};
	UNICODE_STRING name;

	RtlInitUnicodeString(&name, DD_BEEP_DEVICE_NAME_U);
	InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE,
		nullptr, nullptr);

	IO_STATUS_BLOCK ioStatus;
	NTSTATUS status = ::NtOpenFile(&hFile, GENERIC_WRITE, &attr, &ioStatus, 0, 0);
	if (NT_SUCCESS(status))
	{
		BEEP_SET_PARAMETERS params{};

		params.Frequency = freq;
		params.Duration = duration;

		DWORD  bytes;

		/*
		 * Play the sound.
		 */
		printf("Playing freq: %u, duration: %u\n", freq, duration);
		BOOL success = ::DeviceIoControl(hFile, IOCTL_BEEP_SET, &params, sizeof(params),
			nullptr, 0, &bytes, nullptr);
		if (!success)
		{
			DWORD err = ::GetLastError();
			if (err != ERROR_IO_PENDING)
				printf("Failed to play the sound (%d)\n", err);
		}

		/*
		 * The sound starts playing and the call returns immediately.
		 * Wait so that the app doesn't close.
		 */
		::Sleep(duration);
		::CloseHandle(hFile);
	}
	else
		printf("Failed to open the Beep device (status: %lu)\n", status);
}