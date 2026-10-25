#include "PluginEditor.h"

VibeSparkEditor::VibeSparkEditor (VibeSparkProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (640, 520);

    addAndMakeVisible (vibePad);
    vibePad.setPosition (proc.vibeX.load(), proc.vibeY.load());

    vibePad.onPadChanged = [this] (float x, float y) {
        proc.setDarkAndTense (x, y);
        chordStrip.setChords (proc.chordNames);
    };

    addAndMakeVisible (chordStrip);
    chordStrip.setChords (proc.chordNames);
    chordStrip.onStartDragAll = [this] { exportAndDragAllMidi(); };
    chordStrip.onStartDragSingle = [this] (int idx) { exportAndDragSingleMidi (idx); };
    chordStrip.onClickPreviewChord = [this] (int idx) { previewChordClicked (idx); };

    keyBox.addItemList (juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B","Random"}, 1);
    keyBox.setSelectedItemIndex (proc.key.load(), juce::dontSendNotification);
    keyBox.onChange = [this] { proc.setKey (keyBox.getSelectedItemIndex()); chordStrip.setChords (proc.chordNames); };

    voiBox.addItemList (juce::StringArray{"Close Voicing", "Open Voicing"}, 1);
    voiBox.setSelectedItemIndex (proc.open.load(), juce::dontSendNotification);
    voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.generate(false); chordStrip.setChords (proc.chordNames); };

    octBox.addItemList (juce::StringArray{"Oct: -1", "Oct: 0", "Oct: +1"}, 1);
    octBox.setSelectedItemIndex (proc.octaveShift.load() + 1, juce::dontSendNotification);
    octBox.onChange = [this] { proc.octaveShift = octBox.getSelectedItemIndex() - 1; };

    nobassButton.setToggleState (proc.noBass.load(), juce::dontSendNotification);
    nobassButton.onClick = [this] { proc.noBass = nobassButton.getToggleState(); proc.generate(false); };

    strumButton.setToggleState (proc.strum.load(), juce::dontSendNotification);
    strumButton.onClick = [this] { proc.strum = strumButton.getToggleState(); };

    for (auto* comp : {&keyBox, &voiBox, &octBox, &nobassButton, &strumButton})
        addAndMakeVisible (comp);

    startTimerHz (30);
}

VibeSparkEditor::~VibeSparkEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VibeSparkEditor::exportAndDragAllMidi()
{
    juce::File tempMidi = proc.writeMidiFile();
    if (tempMidi.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ tempMidi.getFullPathName() }, false, this);
}

void VibeSparkEditor::exportAndDragSingleMidi (int index)
{
    juce::File tempMidi = proc.writeSingleMidiFile (index);
    if (tempMidi.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ tempMidi.getFullPathName() }, false, this);
}

void VibeSparkEditor::previewChordClicked (int index)
{
    juce::MidiBuffer midiOut;
    proc.previewChord (index, midiOut);
    // Send preview notes to synth or midi out if needed
}

void VibeSparkEditor::timerCallback()
{
    const double newBpm = proc.hostBpm.load();
    const bool newPlaying = proc.hostPlaying.load();
    const float newBeat = proc.beatInBar.load();
    const bool changed = std::abs (newBpm - bpm) > 0.01 || newPlaying != playing
                      || (int) newBeat != (int) beat;
    bpm = newBpm; playing = newPlaying; beat = newBeat;

    const float vx = proc.vibeX.load();
    accent = ui::cold.interpolatedWith (ui::warm, vx);
    chordStrip.setAccent (accent);
    chordStrip.setActive (proc.currentBar.load());
    if (changed) repaint (bpmArea);
}

void VibeSparkEditor::resized()
{
    auto r = getLocalBounds().reduced (20);
    auto header = r.removeFromTop (50);
    bpmArea = header.removeFromRight (180);
    titleArea = header;
    r.removeFromTop (6);

    padArea = r.removeFromTop (210);
    r.removeFromTop (10);
    chordArea = r.removeFromTop (90);
    r.removeFromTop (10);
    controlsArea = r;

    vibePad.setBounds (padArea);
    chordStrip.setBounds (chordArea);

    auto f = controlsArea;
    int w3 = f.getWidth() / 3;
    keyBox.setBounds (f.removeFromLeft (w3 - 6).withHeight (28));
    f.removeFromLeft (6);
    voiBox.setBounds (f.removeFromLeft (w3 - 6).withHeight (28));
    f.removeFromLeft (6);
    octBox.setBounds (f.withHeight (28));

    auto f2 = controlsArea.removeFromBottom (28);
    nobassButton.setBounds (f2.removeFromLeft (160));
    f2.removeFromLeft (10);
    strumButton.setBounds (f2.removeFromLeft (100));
}

void VibeSparkEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
    juce::ColourGradient wash (ui::cold.withAlpha (0.06f), 0.0f, 0.0f,
                               ui::warm.withAlpha (0.04f), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (wash);
    g.fillAll();

    auto t = titleArea;
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold).withKerningFactor (0.28f)));
    g.drawText ("VIBESPARK", t.removeFromTop (28), juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (ui::font (10.0f));
    g.drawText ("NSA PSYCHOLOGICAL CHORD ENGINE // MODERN VIBES", t, juce::Justification::centredLeft);

    auto b = bpmArea.toFloat();
    g.setColour (ui::panel);  g.fillRoundedRectangle (b, 10.0f);
    g.setColour (ui::line);   g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
    auto inner = b.reduced (10.0f, 6.0f);

    auto left = inner.removeFromLeft (85.0f);
    g.setColour (ui::dim);  g.setFont (ui::font (9.0f));
    g.drawText ("HOST BPM", left.removeFromTop (12.0f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (ui::text); g.setFont (ui::font (22.0f));
    g.drawText (juce::String (bpm, 1), left.toNearestInt(), juce::Justification::centredLeft);

    auto top = inner.removeFromTop (inner.getHeight() * 0.5f);
    auto led = top.removeFromLeft (12.0f).withSizeKeepingCentre (8.0f, 8.0f);
    g.setColour (playing ? ui::live : ui::dim);
    g.fillEllipse (led);
    g.setFont (ui::font (9.5f));
    g.drawText (playing ? "SYNC" : "FREE", top.toNearestInt(), juce::Justification::centredLeft);

    const int idx = (int) beat;
    auto dots = inner.withSizeKeepingCentre (4.0f * 12.0f, 8.0f).withX (inner.getX());
    for (int i = 0; i < 4; ++i)
    {
        auto d = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ dots.getX() + 4.0f + (float) i * 12.0f, dots.getCentreY() });
        g.setColour (i == idx ? accent : ui::line);
        g.fillEllipse (d);
    }
}
