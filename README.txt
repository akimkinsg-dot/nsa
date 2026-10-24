1. Создай бесплатный аккаунт на github.com и новый репозиторий (New repository).
2. Загрузи в него ВСЕ файлы из этой папки, включая скрытую папку .github (Add file > Upload files, перетащи всё).
3. Открой вкладку Actions: сборка запустится сама (~10-15 мин).
4. Когда будет зелёная галочка, открой запуск и скачай NSA-Chords-mac.zip или NSA-Chords-win.zip из Artifacts.
5. Распакуй. Mac: положи NSA Chords.vst3 в /Library/Audio/Plug-Ins/VST3, затем в Терминале: xattr -cr "/Library/Audio/Plug-Ins/VST3/NSA Chords.vst3"
   Windows: положи в C:\Program Files\Common Files\VST3.
6. В Ableton: Preferences > Plug-ins > Rescan. Плагин ставится на MIDI-дорожку как инструмент.
Кнопка Preview играет без запуска Live. Блок DRAG MIDI перетаскивается на любую дорожку.
