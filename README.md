
# SpeedyPizza - Progetto di Grafica in OpenGL

## Obiettivo del gioco

Lo scopo del videogame realizzato consiste nell'effettuare il maggior numero possibile di consegne di pizze ai propri clienti in un tempo limitato 
(impostato a 2 minuti). Per raggiungere l'obiettivo, l'utente guider&agrave; la jeep del fattorino di SpeedyPizza in una mappa quadrata che simula un 
ambiente urbano, evitando di collidere con le strutture presenti in essa per non essere rallentato durante le consegne. Per aumentare il livello di 
sfida e rendere l'azione di gioco pi&ugrave; accattivante, nella mappa &egrave; anche possibile raccogliere durante le consegne una bombola di 
<a href="https://it.wikipedia.org/wiki/Ossido_di_diazoto">N20 (a.k.a. NOS)</a>, che raddoppier&agrave; la velocit&agrave; del veicolo per 5 secondi. L'utente potr&agrave; organizzare i
suoi percorsi per le consegne utilizzando la mappa 2D visualizzata nella finestra di gioco, dove oltre alla posizione della vettura e alla disposizione degli
ostacoli, vengono mostrate la posizione della pizza/cliente e quella del NOS. Inoltre, nel pannello in alto a sinistra &egrave; sempre visibile il tempo rimanente,
il numero di consegne effettuate e il tempo di esaurimento dell'effetto del NOS.

![Applicazione](doc/images/snapshot1.png)


## Modalit&agrave; di gioco

Una volta avviata la partita, il giocatore deve raccogliere la prima pizza nella posizione indicata sulla mappa. Una volta raccolta, comparir&agrave; sulla mappa
il segnalino del cliente, a cui la pizza dev'essere consegnata. Una volta effettuata la consegna, una nuova pizza sar&agrave; pronta e si dovr&agrave; ripetere questo 
procedimento pi&ugrave; volte possibile prima dello scadere del tempo. Sulla mappa sono inoltre sempre presenti 3 NOS (rigenerati man mano che vengono raccolti) che &egrave; possibile attivare durante il tragitto 
effettuato dall'utente.

Nella mappa i vari oggetti sono indicati con i seguenti colori:

* ROSSO: veicolo
* BIANCO: ostacoli
* VERDE: pizza
* BLU: cliente
* GIALLO: NOS

![Mappa 2D](doc/images/map.png)

## Descrizione dei componenti grafici 

### Jeep

La jeep di SpeedyPizza &egrave; stata progettata sulla base di un semplice modello reperito in rete, modificandone leggermente la struttura per migliorarne 
l'aspetto. Il modello &egrave; stato scomposto in 3 diversi oggetti: il telaio, la ruota anteriore e quella posteriore (poi duplicate e riposizionate nella fase di
rendering). Alle superfici della vettura sono state applicate diverse texture, in base ai materiali che vanno a comporre l'oggetto per darle un aspetto pi&ugrave; 
realistico, e sul retro &egrave; stato applicato il logo del gioco.

![Jeep1](doc/images/car1.png)
![Jeep2](doc/images/car2.png)

### Costruzioni

Sono state progettate in Blender quattro semplici costruzioni/strutture, ovvero:

* 2 palazzi residenziali
* 1 palazzo per uffici
* 1 torre

Ad ognuna di esse &egrave; stata applicata una diversa texture sulle facciate in base alla propria tipologia, mentre &egrave; stata applicata una texture comune per il tetto
e la base della costruzione.

Le costruzioni posizionate sulla mappa costituiscono gli ostacoli del gioco, che l'utente dovr&agrave; evitare durante le consegne. E' stato infatti implementato
un semplice meccanismo di _collision detection_ che, in base alla posizione della bounding box (BBOX) della jeep e quelle delle costruzioni, 
verifica se la macchina ha urtato una struttura. Per maggiori dettagli, si rimanda alla relativa <a href="#coll_det">sezione</a>. 

![Buildings](doc/images/buildings.png)

### Pizza

Il modello della pizza e le texture associate sono stati reperiti in rete. Nella dinamica di gioco, per raccogliere la pizza e poterla consegnare &egrave; necessario
guidarci abbastanza vicino con la vettura (la vicinanza &egrave; determinata con la relativa funzione di <a href="#distance">distanza</a>). Per attirare l'attenzione dell'utente,
il modello della pizza viene fatto ruotare costantemente sull'asse Y durante il rendering della scena di gioco.

![Pizza](doc/images/pizza.png)

### NOS

Il modello della bombola &egrave; stato reperito in rete, ma la texture in questo caso &egrave; stata applicata generando automaticamente le coordinate con la modalit&agrave;
GL_SPHERE_MAP e utilizzando una texture di tipo "environment mapping", per simulare i riflessi della superficie lucida dell'oggetto. Come per la pizza,
anche questo modello viene fatto ruotare su s&egrave; stesso durante il rendering della scena e bisogna guidarci vicino per poterlo raccogliere e attivare l'effetto del NOS.

![NOS](doc/images/nos.png)

### Cliente

Il modello del Cliente e le texture associate sono stati reperiti in rete. Tramite Blender, si &egrave; modificato il modello (mediante l'applicazione di _bones_) in modo 
da ricordare la posa di un cliente che con la mano fa cenno al fattorino per la consegna. 

![Client](doc/images/old_man.png)

### Banner pubblicitario

Il modello del banner &egrave; stato reperito in rete, ma le texture sono state modificate in modo da mostrare una foto dello sviluppatore insieme a un gruppo di 
amici mentre si trovava nelle campagne del Salento.

![Banner](doc/images/banner.png)

### Mappa

La mappa di gioco consiste in una matrice quadrata di 25x25 "piastrelle" (nello specifico, superifici di tipo GL_QUAD), ognuno dei quali pu&ograve; ospitare uno
degli oggetti tra:

* una costruzione (ostacolo)
* la pizza
* il cliente
* il NOS
* il banner pubblicitario

Le piastrelle presentano una texture di tipo "cemento" o "mattonelle", a seconda che siano vuote oppure ospitino una costruzione: questo serve a dare l'idea 
di un percorso stradale che costeggi le aree abitate, dove la jeep dovrebbe spostarsi. Tuttavia, non &egrave; vietato prendere scorciatoie negli spazi tra le costruzioni,
dando la possibilit&agrave; al giocatore di prendere scorciatoie.

La mappa viene generata in maniera random ad ogni nuova partita, cos&igrave; come la posizione della pizza/cliente ad ogni consegna, del NOS e del banner.
Per quanto riguarda pizza, cliente, NOS e banner, gli oggetti vengono posizionati sempre in una piastrella non occupata da un palazzo, mentre per la modalit&agrave;
di generazione della mappa degli ostacoli si rimanda alla relativa <a href="#random_gen">sezione</a>.

![GenMap](doc/images/gen_map.png)

## Dettagli implementativi

### Caricamento file .obj e .mtl
Quando viene caricato un file .obj, se questi presenta informazioni sui materiali da applicare alle superfici, esse vengono lette dal relativo file
.mtl. Il materiale e la texture da applicare vengono salvate in un vettore che viene riempito man mano che i file .mtl vengono letti per ognuno degli oggetti. 
Per ogni faccia da disegnare di ogni oggetto viene salvato l'indice del materiale da applicare.
Una volta salvate queste informazioni, le varie texture associate ai materiali vengono caricate in memoria con l'apposita funzione _LoadTexture_ (che ne genera la MipMap 2D)
e il relativo indice, da usare con _glBindTexture_, viene aggiornato per ogni materiale. Nella fase di rendering, quando vengono letti i vertici delle 
facce e le relative coordinate delle texture, queste ultime vengono bindate dinamicamente utilizzando l'indice salvato per ogni faccia. Questo meccanismo permette di caricare e applicare le texture
automaticamente utilizzando le informazioni contenute nei file .obj e .mtl.

### Generazione della mappa

Il posizionamento degli ostacoli viene effettuato utilizzando una _matrice di posizioni_ di N_CELLS*N_CELLS (N_CELLS = 25). Dapprima viene riempito un vettore di N_BUILDINGS elementi,
dove N_BUILDINGS rappresenta il numero di palazzi/ostacoli che vogliamo posizionare sulla mappa (N_BUILDINGS &lt;&lt; N_CELLS). Ognuno degli elementi del vettore
&egrave; una mesh corrispondente a una delle 4 tipologie di palazzi (la tipologia &egrave; scelta in maniera random tra quelle disponibili).

Si procede poi al riempimento della matrice in questo modo:
per ogni palazzo, si genera una posizione random nella matrice e, se libera, si assegna l'indice del palazzo a quella posizione. Si procede a posizionare
altri 8 palazzi intorno ad esso (sempre se ogni posizione risulti libera). Cos&igrave; facendo, per ogni posizione scelta in maniera casuale ci saranno altri palazzi nelle
piastrelle che lo circondano, creando dei "centri di aggregazione" che visivamente ricordino una disposizione realistica di una ambiente urbano e
creando degli spazi fra di essi che possano ricordare delle strade.

### Collision detection

Nella computazione dello _step fisico_ di riposizionamento della vettura, si verifica che la posizione di uno qualsiasi dei vertici della BBOX del telaio 
non sia sovrapposto all'area 2D (sul piano XZ) di ognuna delle costruzioni presenti nella mappa. Se ci&ograve; avviene per una qualsiasi delle strutture,
allora l'accelerazione della vettura viene settata a 0 in tutte le direzioni, avendo come effetto quello di fermare la corsa della vettura.
Questo semplice meccanismo pu&ograve; essere utilizzato in quanto le costruzioni sono orientate tutte allo stesso modo e aggiornando i vertici delle BBOX cos&igrave;:

Costruzioni: al momento della generazione della mappa, in base alla posizione nella matrice delle posizioni, si aggiornano i valori dei vertici minimi 
e massimi della BBOX in base alla traslazione effettuata sulla struttura per posizionarla nel mondo;
Vettura: ad ogni computazione dello _step fisico_, si aggiorna la posizione dei vertici nei 3 assi XYZ in base all'accelerazione applicata al veicolo 
(utilizzando i valori relativi alle coordinate del mondo).


### Distanza jeep-oggetti

La distanza della vettura dagli oggetti viene calcolata con la distanza euclidea tra i due rispettivi centri della BBOX, i quali vengono aggiornati in base alla posizione
attuale degli oggetti nel mondo.
