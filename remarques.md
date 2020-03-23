bio-yeast*: mon programme et l'algorithme du papier de recherche donnent le même résultat mais la clique contient plus de sommets que ce qui est dit sur le site 

Les fichiers de graph ne sont pas triés selon un ordre prédéfinit
 - certains ont la première colonne de triée
 - certains ont la deuxième colonne de triée
 - certains ne sont pas du tout triés (bio-celegan-dirs)
 - certains ont des arrête en double (bio-grid-fruitfly)

Résultats par rapport à notre implémentation pour des graphs de http://networkrepository.com/ :
 - bio-celegans-dir : plus petit que sur le site
 - bio-celegans : pas la même clique mais correspond au lower bound du site
 - bio-diseasome : même résultat, cohérent avec le site
 - bio-grid-fruitfly : segmentation fault (notre programme met encore trop de temps pour lire)
 - bio-grid-human : pareil
 - bio-grid-mouse : pareil
 - bio-yeast-protein-inter : même résultat : un sommet de plus que sur le site (on va corriger un siter officiel mwhahahahahahahaha)
 - bio-yeast : pareil