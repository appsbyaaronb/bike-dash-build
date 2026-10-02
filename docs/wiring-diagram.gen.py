# Generates docs/wiring-diagram.svg: every wire ends on a numbered pin.
import sys
o=[]
A=o.append
RED='#d62728'; ORG='#ff7f0e'; PINK='#d63384'; BLK='#222'; GOLD='#b8860b'; BLUE='#1f77b4'
LEDP='#e6550d'; LEDM='#444'; M0='#6f42c1'; M1='#9467bd'; MC='#5b3a8e'; GRN='#2ca02c'; CYA='#17becf'
TEAL='#0e7c86'; BRN='#8c564b'
def box(x,y,w,h,fill='#f4f4f4',rx=6): A(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="#222" stroke-width="1.5"/>')
def t(x,y,s,size=12,anchor='start',fill=None,bold=False,extra=''):
    a='' if anchor=='start' else f' text-anchor="{anchor}"'
    f=f' fill="{fill}"' if fill else ''
    b=' font-weight="bold"' if bold else ''
    A(f'<text x="{x}" y="{y}" font-size="{size}"{a}{f}{b}{extra}>{s}</text>')
def w(pts,color,dash=False,width=2.5):
    d=' stroke-dasharray="8 4"' if dash else ''
    A(f'<polyline points="{" ".join(f"{x},{y}" for x,y in pts)}" fill="none" stroke="{color}" stroke-width="{width}"{d}/>')
def dot(x,y,c): A(f'<circle cx="{x}" cy="{y}" r="4" fill="{c}"/>')

W,H=1500,1040
A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" aria-label="Bench wiring, every wire pin to pin. 12 V supply feeds the Pololu (IN, GND) and the LED driver (VIN, GND). Pololu OUT to NANO pin 2, GND to pin 6. LED driver PWM to pin 21 (GPIO26), GND to pin 9, LED+ to panel 39/40, LED- to panel 31/32. NANO pin 22 (GPIO27) to panel 5 RESET. NANO pin 1 (3V3) to a 1-to-3 splitter feeding panel 6, panel 33 and GPS 3V3. 15-pin breakout pins 14, 15 (3V3) to panel 2, 3; its GND pins 10, 7, 1, 4, 13 to panel 7, 10, 13, 16, 19; MIPI D0, D1, CLK pairs to panel 8/9, 11/12, 17/18. NANO pin 25 GND to panel 34. GPS: GND pin 14, TX to pin 15, RX to pin 13. Touch: SDA pin 3, SCL pin 5, INT pin 16, RST pin 7, VCC pin 17, GND pin 20.">')
A(f'<rect x="0" y="0" width="{W}" height="{H}" fill="#ffffff"/>')
A('<g font-family="Arial, Helvetica, sans-serif" font-size="12" fill="#222">')
t(750,28,'bike-dash bench wiring: every wire, pin to pin',18,'middle',bold=True)

# ---------- wires first (boxes drawn after would hide nothing: wires stop at box edges) ----------
# 12 V bus (x=30) and supply ground bus (x=50)
w([(110,120),(30,120),(30,565),(110,565)],RED)
A(f'<path d="M30,330 H44 A6,6 0 0 1 56,330 H110" fill="none" stroke="{RED}" stroke-width="2.5"/>')
dot(30,330,RED)
w([(110,150),(50,150),(50,535),(110,535)],BLK)
w([(50,305),(110,305)],BLK); dot(50,305,BLK)
t(22,230,'12 V +',11,'middle',RED,extra=' transform="rotate(-90 22 230)"')
t(64,230,'supply −',11,'middle',extra=' transform="rotate(-90 64 230)"')
# LED driver -> NANO (wrap under the driver)
w([(110,355),(75,355),(75,445),(470,445)],BLK)
w([(110,380),(92,380),(92,420),(470,420)],BLUE)
t(290,414,'PWM dim',11,'middle',BLUE)
t(290,459,'driver GND',11,'middle')
# Pololu -> NANO
w([(270,535),(470,535)],ORG); t(370,529,'5 V',11,'middle','#c8600a')
w([(270,565),(470,565)],BLK); t(370,579,'GND',11,'middle')
# LED+ / LED- over the top to the 40P breakout
w([(270,340),(320,340),(320,52),(1235,52),(1235,110)],LEDP)
w([(270,370),(338,370),(338,64),(1165,64),(1165,110)],LEDM,dash=True)
t(640,44,'LED+  (backlight, about 9 V / 260 mA)',11,'middle',LEDP)
t(640,78,'LED−  (backlight return to the driver, NOT to ground)',11,'middle',LEDM)
# GPS
w([(770,150),(900,150),(900,145)],BLK)
w([(770,172),(950,172),(950,145)],GRN)
w([(770,194),(1000,194),(1000,145)],CYA)
# 3V3 pin 1 -> splitter -> GPS 3V3, panel 6, panel 33
w([(770,225),(800,225)],PINK)
w([(880,225),(1050,225),(1050,145)],PINK)
w([(880,250),(1130,250)],PINK)
w([(880,275),(1130,275)],PINK)
# RESET
w([(770,305),(1130,305)],GOLD); t(1010,299,'RESET',11,'middle','#8a6508')
# FFC band
A('<line x1="770" y1="480" x2="900" y2="480" stroke="#9a9a9a" stroke-width="14"/>')
t(835,466,'15-pin FFC cable',11,'middle')
# 15P -> 40P straight rows
rows15=[('14  3V3',PINK,'2  VDD 3.3 V'),('15  3V3',PINK,'3  VDD 3.3 V'),('10  GND',BLK,'7  GND'),
        ('8  D0−',M0,'8  D0N'),('9  D0+',M0,'9  D0P'),('7  GND',BLK,'10  GND'),
        ('2  D1−',M1,'11  D1N'),('3  D1+',M1,'12  D1P'),('1  GND',BLK,'13  GND'),
        ('4  GND',BLK,'16  GND'),('5  CLK−',MC,'17  DCLKN'),('6  CLK+',MC,'18  DCLKP'),('13  GND',BLK,'19  GND')]
ry=[385+24*i for i in range(13)]
for (a,c,b),y in zip(rows15,ry): w([(1030,y),(1130,y)],c)
# NANO pin 25 GND -> panel 34
w([(770,590),(885,590),(885,735),(1130,735)],BLK)
# touch
tr=[('pin 3 (GPIO7)',CYA,'10  SDA'),('pin 5 (GPIO8)',TEAL,'8  SCL'),('pin 16 (GPIO22)',BRN,'9  INT'),
    ('pin 7 (GPIO23)',GOLD,'7  RST'),('pin 17 (3V3)',PINK,'6  VCC (3.3 V)'),('pin 20 (GND)',BLK,'5  GND')]
ty=[615+25*i for i in range(6)]; tx=[870-15*i for i in range(6)]; by=[850+24*i for i in range(6)]
for (a,c,b),y,x,yy in zip(tr,ty,tx,by): w([(770,y),(x,y),(x,yy),(900,yy)],c)
# tails
A('<line x1="1300" y1="450" x2="1340" y2="450" stroke="#9a9a9a" stroke-width="14"/>')
t(1320,436,'tail',10,'middle')
A('<polyline points="1410,790 1410,910 1090,910" fill="none" stroke="#999" stroke-width="10"/>')
t(1250,898,'touch tail (10-pin, separate)',11,'middle','#555')
t(1250,932,'ILI2132A, I2C addr 0x41',10,'middle','#555')
t(1250,946,'VCC is 3.3 V only. Never 5 V.',10,'middle','#b00')

# ---------- boxes ----------
# supply
box(110,90,160,90); t(190,112,'12 V supply',14,'middle',bold=True)
t(190,130,'bench: 12 V 2 A',11,'middle'); t(190,146,'bike: fused ignition 12 V',11,'middle')
t(116,124,'+',13); t(116,154,'−',13)
t(190,168,'both + wires on the + post, both − on the − post',0,'middle')  # placeholder, removed below
o.pop()
# LED driver
box(110,230,160,180); t(190,250,'LD24AJTA LED driver',14,'middle',bold=True)
t(190,266,'eletechsup, 30–900 mA, pot-set',11,'middle'); t(190,281,'set 0.26 A on meter FIRST',11,'middle','#b00')
t(116,309,'GND'); t(116,334,'VIN'); t(116,359,'GND'); t(116,384,'PWM')
t(264,344,'LED+',anchor='end'); t(264,374,'LED−',anchor='end')
t(190,402,'PT4115 (1R0) stand-in: same pads',9.5,'middle','#555')
# Pololu
box(110,470,160,150); t(190,490,'Pololu D36V28F5',14,'middle',bold=True)
t(190,506,'12 V in → 5 V 3.2 A out',11,'middle')
t(116,539,'GND'); t(116,569,'IN'); t(264,539,'OUT',anchor='end'); t(264,569,'GND',anchor='end')
t(190,596,'EN, PG: leave open',10,'middle','#555'); t(190,610,'the two GND pads are joined',10,'middle','#555')
# NANO
box(470,110,300,680,'#eef3fb',8); t(620,134,'Waveshare ESP32-P4-NANO',14,'middle',bold=True)
t(476,152,'pin n = position on header P1',10,fill='#555'); t(476,165,'(name) = label printed on the board',10,fill='#555')
t(476,424,'pin 21 (GPIO26)'); t(476,449,'pin 9 (GND)'); t(476,539,'pin 2 (5V)'); t(476,569,'pin 6 (GND)')
t(476,610,'bench: USB-C can power the board',10,fill='#555'); t(476,623,'instead of the Pololu (leave pins 2, 6 open)',10,fill='#555')
for y,s in ((150,'pin 14 (GND)'),(172,'pin 15 (GPIO21)'),(194,'pin 13 (GPIO20)'),(225,'pin 1 (3V3)'),(305,'pin 22 (GPIO27)'),(590,'pin 25 (GND)')):
    t(764,y+4,s,anchor='end')
for (a,c,b),y in zip(tr,ty): t(764,y+4,a,anchor='end')
A('<rect x="742" y="420" width="28" height="120" fill="#d9e2f3" stroke="#222"/>')
t(756,480,'DSI 15-pin',10,'middle',extra=' transform="rotate(-90 756 480)"')
t(600,345,'C6 radio on GPIO14–19, 54 (internal)',11,'middle','#555')
# splitter
box(800,205,80,84,'#fde8f1'); t(840,243,'3V3 splitter',10,'middle'); t(840,256,'1 in → 3 out',10,'middle')
# GPS
box(860,78,230,67); t(975,96,'SparkFun NEO-M9N GPS',13,'middle',bold=True); t(975,111,'3.3 V only. Never 5 V.',11,'middle','#b00')
for x,s in ((900,'GND'),(950,'TX'),(1000,'RX'),(1050,'3V3')): t(x,138,s,11,'middle')
# 15P
box(900,335,130,365); t(965,353,'15P breakout',13,'middle',bold=True); t(965,367,'(1.0 mm pitch, on the FFC)',9.5,'middle','#555')
for (a,c,b),y in zip(rows15,ry): t(1024,y+4,a,11,'end')
t(965,714,'11 SCL, 12 SDA: not used',10,'middle','#555')
# 40P
box(1130,110,170,680); t(1165,128,'31, 32',11,'middle'); t(1165,141,'LED−',11,'middle'); t(1235,128,'39, 40',11,'middle'); t(1235,141,'LED+',11,'middle')
t(1215,172,'40P breakout',13,'middle',bold=True); t(1215,187,'numbers = panel pin',10,'middle','#555')
t(1136,254,'6  STBYB',11); t(1136,279,'33  L/R',11); t(1136,309,'5  RESET',11)
for (a,c,b),y in zip(rows15,ry): t(1136,y+4,b,11)
t(1136,739,'34  U/D',11)
t(1130,810,'22, 25, 30 GND: no wire (same ground inside the panel)',10,fill='#555')
t(1130,825,'14, 15, 20, 21 (lanes 2–3): open',10,fill='#555')
t(1130,840,'1, 4, 23, 24, 26–29, 35–38 NC: open',10,fill='#555')
# panel
box(1340,110,140,680,'#e8f5e9'); t(1410,135,'Riverdi panel',13,'middle',bold=True)
t(1410,152,'RVT70HSMNWC00-B',10,'middle'); t(1410,168,'7" 1024×600, 850 nit',10,'middle')
t(1410,450,'40-pin FPC tail,',10,'middle','#555'); t(1410,464,'contacts one side:',10,'middle','#555'); t(1410,478,'flip if no picture',10,'middle','#555')
# touch breakout
box(900,800,190,190); t(995,820,'10P touch breakout',13,'middle',bold=True)
for (a,c,b),yy in zip(tr,by): t(906,yy+4,b,11)
t(1084,984,'1–4 (USB): open',10,'end','#555')

# ---------- legend ----------
A('<rect x="20" y="650" width="425" height="150" rx="6" fill="#fff" stroke="#222" stroke-width="1"/>')
t(32,668,'Legend',12,bold=True)
L=[(RED,'12 V',0),(ORG,'5 V',0),(PINK,'3.3 V',0),(BLK,'ground wire',0),(GOLD,'RESET / touch RST',0),(BLUE,'backlight dim PWM',0),
   (LEDP,'LED+ backlight',1),(LEDM,'LED− backlight return',1),(M0,'MIPI pairs (D0, D1, CLK)',1),(GRN,'GPS TX → NANO',1),(CYA,'NANO → GPS RX / touch SDA',1),(TEAL,'touch SCL (brown = INT)',1)]
n=[0,0]
for c,s,col in L:
    x=32+col*205; y=684+n[col]*17; n[col]+=1
    d=' stroke-dasharray="8 4"' if c==LEDM else ''
    A(f'<line x1="{x}" y1="{y}" x2="{x+34}" y2="{y}" stroke="{c}" stroke-width="2.5"{d}/>'); t(x+42,y+4,s,11)
dot(38,790,BLK); t(48,794,'= joined. A hop over a line = not joined. Every wire ends on a numbered pin.',10,fill='#555')

# ---------- P1 pin map ----------
pins={1:('3V3','3V3 splitter'),2:('5V','5 V in'),3:('GPIO7','touch SDA'),4:('5V',''),5:('GPIO8','touch SCL'),6:('GND','Pololu GND'),
7:('GPIO23','touch RST'),8:('GPIO37',''),9:('GND','driver GND'),10:('GPIO38',''),11:('GPIO5',''),12:('GPIO4',''),
13:('GPIO20','GPS RX out'),14:('GND','GPS GND'),15:('GPIO21','GPS TX in'),16:('GPIO22','touch INT'),17:('3V3','touch VCC'),
18:('GPIO24','leave free'),19:('GPIO25','leave free'),20:('GND','touch GND'),21:('GPIO26','dim PWM'),22:('GPIO27','panel RST'),
23:('GPIO32',''),24:('GPIO33',''),25:('GND','panel 34'),26:('GPIO36','')}
fill={'3V3':'#fbd5e6','5V':'#ffe0bf','GND':'#dddddd'}
A('<rect x="20" y="822" width="670" height="204" rx="6" fill="#fff" stroke="#222" stroke-width="1"/>')
t(32,842,'P4-NANO header P1 (2x13): pin number, printed label, what plugs into it',12,bold=True)
x0,cw,ch=32,50,52
for y,start in ((852,2),(908,1)):
    for i in range(13):
        nn=start+2*i; sig,use=pins[nn]; x=x0+i*cw
        f=fill.get(sig,'#fff7c2' if use and use!='leave free' else '#ffffff')
        A(f'<rect x="{x}" y="{y}" width="{cw}" height="{ch}" fill="{f}" stroke="#222" stroke-width="1"/>')
        t(x+cw/2,y+14,str(nn),11,'middle',bold=True); t(x+cw/2,y+28,sig,10,'middle')
        if use: t(x+cw/2,y+43,use,8,'middle','#555')
t(32,980,'Even pins are one row, odd pins the other. Pins 1 and 17 are 3.3 V outputs: never put 5 V on them.',10,fill='#555')
t(32,995,'Pins 18, 19 are the USB serial pins: leave free. Pin 4 (5V) is spare. Source: NANO schematic (hardware.md).',10,fill='#555')
t(32,1010,'3V3 splitter: a 1-to-3 Dupont Y cable, or one row of a mini breadboard. It feeds panel 6, panel 33 and GPS 3V3.',10,fill='#555')
A('</g>'); A('</svg>')
open(sys.argv[1],'w',encoding='utf-8').write('\n'.join(o)+'\n')
