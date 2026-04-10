# DLS2SF2

`DLS2SF2` は DLS Level 1 を中心に、RIFF ベースの DLS サウンドバンクを SoundFont 2 (`.sf2`) へ変換する C++ ツールです。

ライセンスは `MPL-2.0` です。詳細は `LICENSE` を参照してください。

現状の方針は次のとおりです。

- DLS1 で SF2 に表現可能な要素は変換する
- DLS2 拡張は壊さず読める余地を残す
- SF2 で 1:1 に表せない要素は黙って落とさず warning に残す

## ビルド

### CMake

```powershell
cmake -B build
cmake --build build
```

### Visual Studio

`DLS2SF2.sln` を開いてビルドします。

## 実行

```powershell
.\build\dls2sf2_test.exe <input.dls> [output.sf2]
```

`output.sf2` を省略した場合は、入力ファイルと同じ場所に同名の `.sf2` を出力します。

## テスト

```powershell
.\build\dls2sf2_test.exe
```

テスト内容:

- 合成 DLS を使った parser / converter / writer の構造検証
- `ptbl`, `wsmp`, `wlnk`, `art1`, `INFO`, `hydra` の検証
- stereo link, generator, modulator, warning 分類の検証
- 実サンプル DLS の smoke test

## 対応状況

### DLS パース

- `RIFF DLS `
- `colh`
- `ptbl`
- `LIST lins`
- `LIST ins `
- `insh`
- `LIST lrgn`
- `LIST rgn ` / `LIST rgn2`
- `rgnh`
- `wlnk`
- `wsmp`
- `LIST lart` / `LIST lar2`
- `art1` / `art2`
- `LIST wvpl`
- `LIST wave`
- `fmt `
- `data`
- `LIST INFO`

### SF2 出力

- `RIFF sfbk`
- `LIST INFO`
- `LIST sdta`
- `smpl`
- `LIST pdta`
- `phdr`
- `pbag`
- `pmod`
- `pgen`
- `inst`
- `ibag`
- `imod`
- `igen`
- `shdr`

## 変換対応

### サンプル関連

- wave pool (`ptbl`) 解決
- wave / region `wsmp`
- root key, fine tune, attenuation
- loop start / loop end
- `sampleModes`
- `wlnk` からの sample type / sample link
- mono / left / right / linked sample

### instrument / preset 関連

- DLS instrument -> SF2 preset + instrument
- `insh` の bank / program
- drum bank (`0x80000000`) -> bank 128
- region key / velocity range
- key group -> `exclusiveClass`

### articulation

対応済み generator / depth 系:

- attenuation / gain
- pitch
- pan
- chorus / reverb send
- filter cutoff / resonance
- LFO / vibrato frequency / delay
- volume envelope time / sustain
- modulation envelope time / sustain
- key number to env hold / decay
- LFO -> pitch / filter / volume
- EG2 -> pitch / filter
- vibrato -> pitch
- keynumber -> overriding root key

対応済み modulator source:

- velocity
- key number
- pitch wheel
- poly pressure
- channel pressure / mono pressure
- CC1 / CC7 / CC10 / CC11
- CC91 / CC93
- RPN0

対応済み transform:

- none
- concave
- convex
- switch

## 制限事項

以下は SF2 で 1:1 に表しにくいため、warning に分類されます。

- 出力チャンネル系 destination
  - `LEFT`
  - `RIGHT`
  - `CENTER`
  - `LEFTREAR`
  - `RIGHTREAR`
  - `LFE`
- SF2 に直接対応しない内部 modulation source を使う接続の一部
- 未知の DLS destination / source / transform

warning は `SF2File.warnings` と `ICMT` に反映されます。

## DLS2 について

この実装は DLS1 を主対象にしていますが、次の DLS2 拡張は壊さず扱えるようにしています。

- `rgn2`
- `lar2`
- `art2`
- DLS2 source / destination / transform の一部

ただし、DLS2 の全機能を完全再現するものではありません。DLS2 独自表現で SF2 に直接落ちないものは warning として残します。

## 実装構成

- `src/include/dls2sf2.hpp`
  - 公開 API とデータ構造
- `src/src/dls_parser.cpp`
  - DLS RIFF parser
- `src/src/dls_converter.cpp`
  - DLS -> SF2 変換
- `src/src/sf2_writer.cpp`
  - SF2 writer
- `src/src/dls2sf2_internal.*`
  - 非公開ヘルパ
- `test/test.cpp`
  - 回帰テスト / smoke test

## 完了基準

このリポジトリでいう「完成」は次を満たす状態です。

- DLS1 で SF2 に表現可能な要素は変換済み
- SF2 で表現不能な要素は warning として可視化
- 生成される SF2 は仕様上妥当な構造を持つ
- 合成テストと実サンプル smoke test が通る
