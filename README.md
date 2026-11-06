# Voronoi Studio

以 C++17 / Qt Widgets 實作 Voronoi Diagram 的 divide-and-conquer 視覺化工具。對應 `Algo_termproject.pdf` 的程式介面與輸出入要求；原點在畫布左上，x 向右、y 向下，合法座標為整數 `[0, 600]`。

## 執行

Windows 直接執行 `dist/VoronoiStudio/VoronoiDiagramAlgorithm.exe`，或雙擊根目錄的 `run.cmd`。第一次沒有執行檔時，`run.cmd` 會嘗試使用已安裝的 Qt / MinGW 建置並部署。可將整個 `dist/VoronoiStudio` 資料夾複製到另一台 Windows 電腦，必須一起攜帶 DLL 與 `platforms` 等子資料夾。

也可以用 Qt Creator 開啟 `CMakeLists.txt`，選擇 Desktop Qt MinGW kit 後 Build / Run。`mainwindow.ui` 為原始專案留下的空白表單，實際介面由 `mainwindow.cpp` 建立。

## 操作

| 操作 | 功能 |
| --- | --- |
| 畫布左鍵 / 右鍵 | 新增整數座標 / 刪除鄰近點 |
| 座標輸入 / 復原一點 | 精確新增 x、y / 移除最後加入的點 |
| Step by step / F10 | 執行至下一個合併前暫停點，或顯示該次合併完成的結果 |
| Run / F5 | 從當前遞迴狀態繼續執行至最終結果 |
| 自動播放步驟 | 每 700 ms 執行下一步，可暫停或改用 Run |
| 重新演算 / 清空 | 保留點集重設演算法 / 清空點集 |
| 開啟檔案 / Ctrl+O | 自動辨識測試輸入與 P/E 結果檔 |
| 上一組 / 下一組 | 手動切換多組測資；最後再按下一組會顯示停止訊息 |
| 儲存結果 / Ctrl+S | 儲存已完成的 P/E 結果；尚未完成時停用 |
| 儲存輸入 | 儲存目前點集為可重新讀入的測試輸入檔 |

畫布實際繪圖區至少 600 × 600，視窗放大時等比例顯示；小螢幕可捲動工作區，保持畫布尺寸並存取所有控制項。合併前以藍色、橘色顯示左右子圖；虛線表示凸包、共同切線與分割位置。合併後以綠色顯示新建立的分界鏈。最終結果僅顯示最終 Voronoi 邊。可切換格線、凸包／切線、合併鏈與座標標籤；右側列出排序後的座標、目前步驟、邊數與演算時間。

## 文字檔格式

測試輸入支援多組資料、空行、UTF-8 BOM，以及 `#` 或 `//` 開頭和行尾的註解。讀到點數 `0` 時，忽略其後全部內容；EOF 也可結束檔案。

```text
# 四點測資
4
193 64
193 370
103 200
283 200
0
```

每組資料讀入後停留等待操作；不會在演算結束時自動跳到下一組。重複座標保留於輸入計數中，核心合併重複點並顯示合併數量。單一點的圖沒有邊。

輸出前半為 `P x y`，後半為 `E x1 y1 x2 y2`，兩者各自依字典序排序。每條邊的兩端也依字典序正規化；不輸出畫布外的射線部分或畫布邊框。內部計算與繪圖保留浮點精度，文字端點依 PDF 整數範例向下取整，因此量化誤差小於一個座標單位；量化後的重複邊、零長邊會移除。`examples/pdf-example.txt` 與 PDF 範例逐行相同。

讀入 P/E 檔會直接畫出檔案指定的線段，不重新計算；可連續開啟不同的結果檔比較。若要對其 P 點重新計算，按「重新演算」。無效輸入會顯示行號與錯誤原因，原有場景保持可用。

## 演算法與資料結構

核心位於 `voronoi.h/.cpp`，不依賴 Qt 或第三方幾何套件。

1. 依 `(x, y)` 排序並合併重複點；按 x 遞迴分割。相同 x 的整欄留在同一側，確保左右點集嚴格分離。
2. 1～3 點直接以中垂線和最近鄰半平面求解。全共線集合由相鄰點的中垂線直接建立。
3. 每個子圖保存點索引、無限邊的參數區間、鄰接串列與凸包。無限直線、射線與線段都以 `midpoint + t * perpendicular` 統一表示，**核心不在中途裁切到畫布**，以免遺失畫布外、之後又影響合併的頂點。
4. 由上共同切線對應的無限 Voronoi 邊起始，在左右兩個子圖的 cell 交界上沿中垂線往下追蹤。於頂點經由子圖鄰接串列切換最近點對，直到下共同切線；共圓頂點會同時切換多個候選 cell，略過零長邊。
5. 使用合併鏈產生的新限制裁去子圖失效的邊，保留有效子圖邊並加入分界鏈，形成上一層圖。只在繪圖／輸出時將無限邊裁至 `[0, 600]²`。
6. 遞迴工作保留在可恢復的動作序列中；Step 與 Run 操作同一組已求解子圖，Run 不會重算已完成的 merge。

數值使用 `long double`，距離比較改寫為相對座標的線性式以降低遠處頂點造成的相消。若遇到追蹤無法辨識的數值退化，會以左右 child cell 的交集建立同一條合併鏈；此保護路徑次數可由 `fallbackMerges()` 觀察，介面也會在合併時揭露。這仍是遞迴子圖合併，不會改用整個點集重建。

此實作遵循分割、共同切線、合併鏈與裁邊的流程，但**不宣稱具有課本最佳化版本嚴格的 O(n log n) 最壞時間界**：高次數頂點的候選 cell 組合、鄰接限制掃描、同 x 分割不平衡，以及退化保護路徑都可能增加時間。完整保留子圖與顯示資料也比只保留最終結果耗用更多記憶體；一般平衡案例為 O(n log n) 儲存量。時間統計包含產生顯示 frame 的成本，並非純幾何核心效能。

## 建置與測試

本次環境：Windows、Qt 6.9.3、MinGW GCC 13.1.0、CMake、Ninja。亦保留 Qt 5 Widgets 的 CMake 與滑鼠事件相容分支，但本次未以 Qt 5 編譯驗證。

```powershell
# 使用已安裝 Qt；依電腦調整路徑
./scripts/build.ps1 -QtRoot D:/src/qt/6.9.3/mingw_64 -Deploy
```

一般 CMake 用法：

```powershell
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt-kit-path>
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure
```

若直接使用 CMake，先把對應 Qt 與 MinGW 的 `bin` 放進目前 shell 的 PATH。`build.ps1` 會自動完成這件事。MinGW 使用 `-save-temps=obj`，避開部分 Windows MinGW 對含中文 TEMP 路徑的限制。

- `geometry_and_io`：獨立 O(n³) oracle 驗證每條線段的鄰接點與端點，固定亂數種子的 1,200 組測資，加上 29 組空集合、單點、重複點、共線、近共線、方格、共圓、邊界案例；逐步排程、Run 接續、註解／終止符、錯誤資料與 PDF 範例輸出。
- `course_test_data`：PDF 連結之課程官方 20 組測資，逐組與獨立 oracle 比對；座標存於 `examples/course-tests.in`，保留終止符後的刻意干擾資料。
- `ui_workflow`：Qt Test 驗證滑鼠輸入／刪除、Step → Run、至少 600 畫布、中文路徑儲存、輸入與結果檔、下一組／上一組／終止、原樣顯示 E 線段。Qt Test 模組可用時才建立。
- 視覺檢查截圖：執行 UI 測試後，存於 `build/qa/`。
- 範例輸入：`examples/regression.in`；結果範例：`examples/pdf-example.txt`。

另外可執行 `build/release/voronoi_tests.exe --benchmark` 取得固定種子的大量點測試（檢查所有輸出邊中點的最近鄰條件）。本次本機 Release 執行的示例數據如下，時間包含 frame 建立，但不含初始化排序與輸入去重；並非跨電腦效能保證：

| 不重複點數 | 畫布內線段 | 合併次數 | 退化保護 | 演算時間 ms |
| ---: | ---: | ---: | ---: | ---: |
| 100 | 267 | 35 | 0 | 2.70 |
| 997 | 2884 | 384 | 0 | 39.68 |
| 4963 | 14597 | 600 | 0 | 224.89 |

部署產物位於 `dist/VoronoiStudio/`，包含必要的 Qt／MinGW runtime。Qt 動態函式庫會使整個軟體超過 PDF 建議的 5 MB 壓縮大小；交付前須依課程允許方式安排附件。這次只驗證本機執行與移除 Qt PATH 的部署啟動，尚未在另一台乾淨 Windows 10 電腦實測。

## 繳交前補充

目前原始碼的版權註解保留待填的學號、中英文姓名，請填入真實資料。以下指令可產生第一行為 `// $LAN=C$`、具有檔名分隔註解的程式原始碼合併檔，並與目前原始碼保持一致：

```powershell
./scripts/export-source.ps1 -StudentId YOUR_ID -ChineseName "你的姓名" -EnglishName "Your Name"
```

輸出至 `dist/source/YOUR_ID.txt`。本次工作提供程式、測試、建置與部署；PDF 所要求的個人 HTML 成果報告與個人心得需另行完成。AI 輔助方式已記錄於 `docs/AI_USAGE.md`，可供整理報告時參考。課程輸入格式依 PDF 所連結的 [官方格式說明](https://nsysucbyang.github.io/my-site/course/algo/vd_testdata.htm) 驗證。
