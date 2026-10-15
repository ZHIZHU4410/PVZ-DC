"""Render the original RevengeSword plant icon without external game assets."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

S = 4
OUT = Path(__file__).parent / "upstream/mods/evilsword/resources/images/RevengeSword.png"

def pts(values):
    return [(round(x*S), round(y*S)) for x, y in values]

im = Image.new("RGBA", (80*S, 80*S), (0, 0, 0, 0))
d = ImageDraw.Draw(im)

def ellipse(box, fill, outline=(23, 48, 12, 255), width=2):
    d.ellipse(tuple(round(v*S) for v in box), fill=fill, outline=outline, width=width*S)

def polygon(points, fill, outline=(26, 52, 15, 255), width=2):
    pp=pts(points)
    d.polygon(pp, fill=fill)
    d.line(pp+[pp[0]], fill=outline, width=width*S, joint="curve")

# Roots and leaves anchor the sprite at ground level.
polygon([(36,61),(14,70),(11,76),(36,72),(42,69)], (49,157,26,255))
polygon([(42,65),(70,67),(77,76),(49,72)], (53,177,30,255))
ellipse((25,64,50,76),(77,190,31,255))
d.arc((8*S,67*S,39*S,78*S),185,345,fill=(125,232,47,255),width=2*S)
d.arc((43*S,66*S,77*S,77*S),190,345,fill=(125,232,47,255),width=2*S)

# Heavy leaf cuirass, with a bark chest plate and broad shoulder guards.
ellipse((22,19,61,67),(44,141,36,255),width=3)
polygon([(21,28),(16,31),(14,51),(28,56),(34,30)],(48,126,39,255),width=3)
polygon([(51,29),(65,34),(64,51),(54,58),(48,39)],(52,143,38,255),width=3)
polygon([(27,32),(48,26),(57,36),(53,62),(39,67),(25,57)],(121,97,54,255),(60,55,30,255),3)
polygon([(29,36),(47,31),(52,39),(49,59),(39,63),(30,54)],(163,130,67,255),(85,67,35,255),2)
polygon([(35,39),(44,35),(48,43),(43,51),(33,47)],(193,155,81,255),(101,76,35,255),1)

# Damage-lit seams on the armor imply its retaliation state.
for path in [[(42,42),(38,48),(43,53),(38,60)],[(51,37),(48,42),(51,48)],[(25,43),(29,48),(27,54)]]:
    d.line(pts(path),fill=(119,31,24,255),width=3*S,joint="curve")
    d.line(pts(path),fill=(255,112,38,255),width=S,joint="curve")

# Face and brow; thick outline remains legible at card scale.
ellipse((21,9,59,39),(96,183,39,255),width=3)
ellipse((25,14,56,35),(137,210,55,255),(45,98,23,255),1)
polygon([(18,20),(24,7),(44,4),(60,13),(62,24),(52,18),(47,12),(35,18)],(65,150,42,255),width=3)
polygon([(18,20),(26,10),(38,7),(30,17)],(125,220,55,255),(64,140,35,255),1)
ellipse((31,20,39,29),(255,250,207,255),(43,62,22,255),1)
ellipse((44,20,52,29),(255,250,207,255),(43,62,22,255),1)
ellipse((35,24,38,28),(25,39,16,255),(25,39,16,255),1)
ellipse((47,24,50,28),(25,39,16,255),(25,39,16,255),1)
d.arc((35*S,25*S,49*S,35*S),10,155,fill=(39,83,21,255),width=2*S)

# Dead Cells' RevengeSword icon is a rusty red/orange blade with a hot yellow
# edge and a blue grip. Reinterpret those recognizable elements in plant form.
# The source card icon is only a read-only visual reference, not an asset here.
polygon([(56,49),(61,45),(67,54),(63,59)],(32,77,140,255),(17,39,77,255),2)
polygon([(60,45),(64,41),(70,47),(66,51)],(53,112,178,255),(19,42,82,255),2)
polygon([(60,40),(64,36),(69,5),(74,1),(77,8),(72,38),(67,44)],(116,30,28,255),(35,30,42,255),2)
polygon([(63,38),(68,10),(74,3),(74,11),(69,37),(66,40)],(232,80,22,255),(149,42,22,255),1)
polygon([(67,35),(70,11),(74,6),(72,19),(69,34)],(255,194,47,255),(241,102,20,255),1)
polygon([(68,5),(74,1),(77,8),(74,13),(73,6)],(255,214,43,255),(174,68,21,255),1)
polygon([(61,38),(69,42),(68,47),(59,43)],(29,82,149,255),(15,37,79,255),2)
polygon([(58,43),(65,46),(64,49),(57,47)],(255,162,38,255),(134,45,28,255),1)
ellipse((62,52,67,57),(52,124,197,255),(18,48,89,255),1)

# Small highlight dots keep the chest readable at 80x80.
ellipse((31,16,34,19),(202,249,113,255),(202,249,113,255),1)
im = im.resize((80,80), Image.Resampling.LANCZOS)
OUT.parent.mkdir(parents=True, exist_ok=True)
im.save(OUT)
print(OUT)
