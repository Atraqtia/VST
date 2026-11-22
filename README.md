# KOJI TADOKORO Thicc Box

JUCE で作った 2 ノブ(FATNESS / MIX)の VST3 サチュレーション+コンプ。

信号の流れ: コンプ → 4x オーバーサンプリングの tanh サチュレーション → リミッター → ドライ/ウェットのミックス

## ビルド

JUCE 8.0.8 をこのフォルダの隣(`../JUCE`)に置いて、次を実行します。

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

GitHub Actions でもビルドされ、結果は Artifacts からダウンロードできます。

JUCE は GPLv3(無償利用時)です。配布する場合はソースの公開が必要です。
