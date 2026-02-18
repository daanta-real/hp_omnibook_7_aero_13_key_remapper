# HP Omnibook 7 Aero 13 Key Remapper (Capture to Insert)

<p align=center>
  <img height="400" alt="image" src="https://github.com/user-attachments/assets/9061a955-0826-4ed2-811b-b2c408d9cd7f" />
</p>

- This tool corrects the awkward keyboard layout (missing Insert key) of the HP Omnibook 7 Aero 13.

  HP Omnibook 7 Aero 13의 해괴한 키보드 레이아웃(Insert 키 부재)을 교정하기 위한 도구

---

### 1. Overview & Purpose (개요 및 목적)
- **Problem**: The 'Capture' key, which forces `Alt+Win+S`, is placed where the `Insert` key should be.

  `Insert` 키가 있어야 할 자리에 `Alt+Win+S`를 강제 전송하는 '캡쳐' 키가 박혀 있음
  
- **Solution**: Intercepts the hardware-level Capture key signal and remaps it to the `Insert` key.

  하드웨어 레벨의 캡쳐 키 신호를 가로채 `Insert` 키 입력으로 변환

---

### 2. Features (특장점)

- **Low-Level Hooking**

  Extremely fast and stable by intercepting events at the OS level using Windows API.
  
  Windows API를 사용하여 OS 최하단에서 이벤트를 가로채므로 매우 빠르고 안정적
  
- **Native Feel**

  Provides a natural typing experience, as if pressing a physical key.
  
  하드웨어 키를 직접 누르는 것 같은 자연스러운 입력감을 제공
  
- **Modifier Support**

  Fully supports all combination keys like `Ctrl+Insert` and `Shift+Insert`.
  
  `Ctrl+Insert`, `Shift+Insert` 등 모든 조합키가 완벽하게 작동
  
- **Stability**

  Minimal resource usage and high stability, written in C++.
  
  C++로 작성되어 리소스 점유율이 거의 없으며 충돌이 적음

---

### 3. Usage (사용법)
1. Run the built `.exe` file.
   
   빌드된 실행 파일(`.exe`)을 실행하기.

2. While running, the 'Capture' key functions as the `Insert` key.

   프로그램이 실행 중일 때 '캡쳐' 키는 `Insert` 키로 작동함.
   
3. **To exit, you must manually terminate the process in the Task Manager (Ctrl+Shift+Esc).**

   **종료하려면 작업 관리자(Ctrl+Shift+Esc)에서 해당 프로세스를 찾아 직접 종료해야 함.**

---

### 4. Limitations (한계점)

- **No Auto-Repeat**

  Due to the nature of low-level interception, key holding (auto-repeat) is not supported.
  
  로우레벨 가로채기 방식의 특성상, 키를 길게 눌러도 연속 입력(Holding)은 지원되지 않음

---

### 5. License (라이선스)

- IDK WTFPL

---

### 6. Author (저작자)

- Daanta ([https://github.com/daanta-real](https://github.com/daanta-real))
