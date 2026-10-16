# Placebo EQ — Meteora Plugins

EQ passivo stile Pultec EQP-1A, voicing calibrato sulle misure di un Klark Teknik EQP-KT.
Formati: VST3 (Windows/macOS) e AU (macOS). Build automatica con GitHub Actions.

## Controlli
- **Low Frequency** 20 / 30 / 60 / 100 / 200 Hz, **Boost** (max ~13.5 dB) e **Atten** (max ~17.5 dB)
- **High Frequency** 3 / 4 / 5 / 8 / 10 / 12 / 16 kHz, **Boost** (max ~18 dB), **Bandwidth** sharp → broad
- **Atten Sel** 5 / 10 / 20 kHz, **Atten** (max ~16 dB)
- **Drive** saturazione valvolare (armoniche pari), **Output** ±12 dB
- **IN**: inserisce/esclude la rete EQ (con dissolvenza, niente click); la spia rossa si spegne quando l'EQ è escluso

Il logo Meteora si illumina quando il plugin è acceso e si spegne quando viene bypassato dall'host.

Doppio clic su una manopola = valore di default. Finestra ridimensionabile dall'angolo.

## Build
Basta fare push: il workflow in `.github/workflows/build.yml` compila e pubblica gli artifact
(scheda **Actions** → ultima run → *Artifacts*).
