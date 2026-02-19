# Kolemak IME Build Makefile
#
# 요구 사항:
#   - CMake 3.16+
#   - Visual Studio 2022 (또는 2019) with C/C++ workload
#
# 사용법:
#   make build     - DLL 빌드 (64비트 + 32비트)
#   make install   - IME 등록 (관리자 권한 필요)
#   make uninstall - IME 등록 해제 (관리자 권한 필요)
#   make clean     - 빌드 결과물 삭제

IME_DIR       := ime
BUILD64_DIR   := $(IME_DIR)/build
BUILD32_DIR   := $(IME_DIR)/build32
DLL64_PATH    := $(BUILD64_DIR)/Release/kolemak.dll
DLL32_PATH    := $(BUILD32_DIR)/Release/kolemak.dll
CMAKE_GEN     ?= "Visual Studio 17 2022"

.PHONY: build build64 build32 install uninstall clean

build: build64 build32

build64:
	@echo "[*] Building Kolemak IME (x64)..."
	@mkdir -p $(BUILD64_DIR)
	cd $(BUILD64_DIR) && cmake .. -G $(CMAKE_GEN) -A x64
	cd $(BUILD64_DIR) && cmake --build . --config Release
	@echo "[+] Build succeeded: $(DLL64_PATH)"

build32:
	@echo "[*] Building Kolemak IME (x86)..."
	@mkdir -p $(BUILD32_DIR)
	cd $(BUILD32_DIR) && cmake .. -G $(CMAKE_GEN) -A Win32
	cd $(BUILD32_DIR) && cmake --build . --config Release
	@echo "[+] Build succeeded: $(DLL32_PATH)"

install: $(DLL64_PATH) $(DLL32_PATH)
	@echo "[*] Registering Kolemak IME..."
	regsvr32 //s "$(CURDIR)/$(DLL64_PATH)"
	regsvr32 //s "$(CURDIR)/$(DLL32_PATH)"
	@echo "[+] Kolemak IME registered (x64 + x86)."

uninstall:
	@echo "[*] Unregistering Kolemak IME..."
	regsvr32 //u //s "$(CURDIR)/$(DLL64_PATH)"
	regsvr32 //u //s "$(CURDIR)/$(DLL32_PATH)"
	@echo "[+] Kolemak IME unregistered."

clean:
	@echo "[*] Cleaning build artifacts..."
	rm -rf $(BUILD64_DIR) $(BUILD32_DIR)
	@echo "[+] Clean complete."

$(DLL64_PATH):
	$(MAKE) build64

$(DLL32_PATH):
	$(MAKE) build32
