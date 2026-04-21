v3.4.0 [2026-04-22]

- Forked from Winyl v3.3.1. Project renamed to Winyl minus.
- ビルド環境をVS2022に更新、基本的にWindows 10以降に動作環境を限定
- ビルド構成を64bit版のみに変更
- 3rd Party PackageをVcpkg manifest経由で取得するようにした
- 3rd Party Packageの最新版（TagLib 2.2 etc...）で動作するよう修正
- radio stationsの設定をヘッダファイルから起動時にXMLファイルを読み込む形に変更（radio_stations.xml）
- BASS Error他の情報を出力するAudio Logコンソールを追加
- 最新版BASS/BASSASIO/BASSWASAPI/BASSMIXで動作するよう修正
- WinylWnd.cppを4分割

❗PackSkinは未修正


v3.3.1 [15.10.2018]

- Added x64 version.
- Updated radio stations.
- Updated lyrics providers.
- Changed behaviour when reloading cover arts.
- Fixed blurry custom icon.
- Changed the custom icon name to Main.ico instead of Winyl.ico.

v3.3 [02.03.2018]

- Winyl is open-source now. The source code is available on GitHub.
- License changed to GPLv3.
- Fixed WASAPI Exclusive audio output.

---
Old changelog is located in src/changelog.old.txt
