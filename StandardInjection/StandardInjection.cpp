// StandardInjection.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <Windows.h>
#include <memory>

class SafeHandle {
public:
	SafeHandle(const SafeHandle&) = delete;
	SafeHandle& operator=(const SafeHandle& other) = delete;
	
	SafeHandle(HANDLE handle) {
		this->m_ManagedHandle = handle;
	}
	
	SafeHandle(SafeHandle&& other) {
		this->m_ManagedHandle = other.m_ManagedHandle;
		other.m_ManagedHandle = INVALID_HANDLE_VALUE;
	}
	
	SafeHandle& operator=(SafeHandle&& other) {
		if(this != &other) {
			this->m_ManagedHandle = other.m_ManagedHandle;
			other.m_ManagedHandle = INVALID_HANDLE_VALUE;
		}
		
		return *this;
	}
	
	~SafeHandle() {
		if(this->m_ManagedHandle != NULL && this->m_ManagedHandle != INVALID_HANDLE_VALUE)
			CloseHandle(this->m_ManagedHandle);
	}
	
public:
	HANDLE get() const { return this->m_ManagedHandle; }
	
private:
	HANDLE m_ManagedHandle;
};

class Allocator {
public:
	Allocator(void* memory, HANDLE proc = NULL) {
		this->_memory = memory;
		this->_optionalProcess = proc;
	}
	
	~Allocator() {
		if(this->_memory) {
			if(this->_optionalProcess) {
				VirtualFreeEx(this->_optionalProcess, this->_memory, 0, MEM_RELEASE);
			} else {
				VirtualFree(this->_memory, 0, MEM_RELEASE);
			}
		}
	}
	
public:
	void* get() const { return _memory; }
	
private:
	HANDLE _optionalProcess
	void* _memory;
};

int main() {
	// path to our dll
	LPCSTR DllPath = "D:\\projects\\standardinjection\\release\\testlib.dll";

	// Open a handle to target process
	SafeHandle hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 17344);
	
	if(!hProcess)
		return 0;

	// Allocate memory for the dllpath in the target process
	// length of the path string + null terminator
	Allocator mem(VirtualAllocEx(hProcess.get(), nullptr, strlen(DllPath) + 1,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), hProcess.get());
	
	if(!mem.get())
		return 0;

	// Write the path to the address of the memory we just allocated
	// in the target process
	if(!WriteProcessMemory(hProcess.get(), mem.get(), DllPath,
		strlen(DllPath) + 1, nullptr))
		return 0;

	// Create a Remote Thread in the target process which
	// calls LoadLibraryA as our dllpath as an argument -> program loads our dll
	SafeHandle hLoadThread = CreateRemoteThread(hProcess.get(), nullptr, 0,
		reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("Kernel32.dll"),
			"LoadLibraryA")), mem.get(), 0, nullptr);
	
	if(!hLoadThread)
		return 0;

	// Wait for the execution of our loader thread to finish
	WaitForSingleObject(hLoadThread, INFINITE);

	std::cout << "Dll path allocated at: " << std::hex << pDllPath << std::endl;
	std::cin.get();

	return 0;
}
