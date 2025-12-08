# Laboratoire 05 - Système de Partage de Vélos

**Auteurs :** Perret Jonatan et Marcuard Adrien
**Date :** 8 décembre 2025

## Introduction au problème

Ce laboratoire simule un système de partage de vélos dans une ville comportant plusieurs sites équipés de bornes. Le système doit gérer de manière synchronisée les actions de plusieurs acteurs concurrents :

- **Des usagers (Person)** : Ils empruntent un vélo à un site, se déplacent vers un autre site pour le déposer, puis marchent vers un autre site pour recommencer le cycle. Chaque usager a une préférence pour un type de vélo spécifique (VTT, Route ou Gravel).

- **Une camionnette de redistribution (Van)** : Elle effectue des tournées régulières pour équilibrer le nombre de vélos entre les sites et le dépôt central. Elle charge des vélos au dépôt, visite chaque site pour retirer les vélos en surplus ou déposer des vélos manquants, puis retourne au dépôt.

- **Des stations de vélos (BikeStation)** : Chaque site possède une capacité limitée de bornes (par défaut 6). Les stations doivent gérer l'accès concurrent des usagers et de la camionnette.

Le défi principal est d'assurer la synchronisation correcte entre tous ces acteurs pour éviter les interblocages (deadlocks), les conditions de course (race conditions) et garantir un partage équitable des ressources. Le système comporte 3 types de vélos différents, 8 sites utilisateurs, 1 dépôt, 10 usagers et 35 vélos au total.

## Choix d'implémentation

### Architecture générale de synchronisation

La synchronisation du système repose sur la classe **BikeStation** qui implémente un moniteur thread-safe utilisant des mutex et des variables de condition. Chaque station stocke les vélos dans des deques séparées par type, permettant une gestion FIFO pour chaque catégorie.

### Mécanisme de protection des ressources critiques

Un mutex unique protège l'ensemble des données d'une station (nombre de vélos, contenus des deques). Toute opération de lecture ou modification des vélos doit acquérir ce mutex. Cette approche garantit la cohérence des données mais nécessite une attention particulière pour éviter les interblocages.

### Variables de condition pour l'attente active

Le système utilise deux types de variables de condition par type de vélo :

1. **bikeAdded** : Signale l'ajout d'un vélo d'un type donné. Les usagers en attente d'un type spécifique s'y bloquent.
2. **bikeRemoved** : Signale le retrait d'un vélo, libérant potentiellement une borne. Les usagers souhaitant déposer un vélo s'y bloquent lorsque la station est pleine.

Cette séparation par type permet de réveiller uniquement les threads concernés par un changement particulier, optimisant l'efficacité du système.

### Gestion de la capacité des stations

La méthode **putBike** vérifie d'abord si la station est pleine. Si c'est le cas, le thread appelant se bloque sur la variable de condition correspondant au type de vélo qu'il souhaite déposer. Lorsqu'un vélo est retiré, la variable appropriée est notifiée, permettant à un thread en attente de se réveiller et de tenter à nouveau l'insertion.

Inversement, **getBike** attend qu'un vélo du type demandé soit disponible. Cette attente est spécifique au type, ce qui permet aux usagers de n'attendre que leur type préféré sans être réveillés inutilement par l'ajout d'autres types.

### Algorithme de la camionnette

La camionnette suit un cycle prédictible :
1. Charge jusqu'à 4 vélos au dépôt
2. Visite séquentiellement chaque site
3. À chaque site, évalue le nombre de vélos présents
4. Si le site a plus de 4 vélos (seuil = BORNES - 2 = 4), elle retire les excédents
5. Si le site a moins de 4 vélos, elle dépose des vélos en prioritisant les types manquants
6. Retourne au dépôt pour décharger les vélos restants

Cette stratégie de rééquilibrage vise à maintenir chaque site proche du seuil de 4 vélos tout en assurant une diversité de types disponibles.

### Opérations par lot (addBikes et getBikes)

Pour optimiser les opérations de la camionnette, deux méthodes permettent de manipuler plusieurs vélos à la fois :

- **getBikes** : Retire jusqu'à N vélos en parcourant les types dans l'ordre et en appliquant FIFO au sein de chaque type. Cette méthode ne bloque pas ; elle retourne simplement les vélos disponibles.

- **addBikes** : Tente d'ajouter un vecteur de vélos. Les vélos qui ne peuvent pas être ajoutés (station pleine) sont retournés dans le vecteur résultat. Actuellement implémentée sans attente, ce qui est approprié pour la camionnette qui ne doit pas se bloquer indéfiniment.

### Terminaison propre du système

La méthode **ending** permet d'arrêter gracieusement le système. Elle positionne un flag **shouldEnd** et réveille tous les threads en attente sur toutes les variables de condition. Chaque méthode bloquante vérifie ce flag et retourne immédiatement (avec nullptr pour getBike) si la terminaison est demandée. Cela évite que des threads restent bloqués indéfiniment lors de l'arrêt de l'application.

### Comportement des usagers

Chaque usager possède une préférence de type de vélo définie aléatoirement à la construction. Il suit un cycle simple :
1. Attend et prend un vélo de son type préféré au site courant
2. Se déplace en vélo vers un autre site choisi aléatoirement
3. Dépose le vélo au site de destination
4. Marche vers un nouveau site aléatoire
5. Recommence le cycle

Le choix d'attendre uniquement son type préféré peut potentiellement créer des déséquilibres si la distribution des préférences n'est pas uniforme, mais reflète un comportement réaliste d'utilisateurs ayant des préférences marquées.

## Tests effectués

### Test de fonctionnement normal

**Objectif** : Vérifier que le système fonctionne correctement avec les paramètres par défaut (8 sites, 10 usagers, 35 vélos, 3 types).

**Procédure** : Lancer l'application et observer pendant plusieurs minutes le comportement des usagers et de la camionnette.

**Résultats attendus** : 
- Les usagers doivent pouvoir emprunter et déposer des vélos sans blocage
- La camionnette doit effectuer ses tournées régulièrement
- Les compteurs de vélos aux différents sites doivent fluctuer de manière cohérente
- Aucun site ne doit rester définitivement vide ou plein

### Test de saturation des sites

**Objectif** : Vérifier le comportement lorsque plusieurs usagers tentent de déposer des vélos simultanément sur un site saturé.

**Procédure** : Observer particulièrement les sites ayant 6 vélos (capacité maximale). Vérifier que les usagers se bloquent correctement en attente d'une borne libre.

**Points de vigilance** :
- Les usagers doivent se débloquer lorsque la camionnette ou un autre usager retire un vélo
- Aucun dépassement de capacité ne doit survenir
- Les notifications doivent réveiller les bons threads

### Test de pénurie de vélos

**Objectif** : Observer le comportement quand un ou plusieurs types de vélos deviennent rares.

**Procédure** : Observer les situations où un site ne possède aucun vélo d'un type spécifique recherché par un usager.

**Résultats attendus** :
- L'usager concerné doit se bloquer en attente
- La camionnette devrait éventuellement amener un vélo du type manquant
- L'ordre FIFO des attentes doit être respecté

### Test de terminaison propre

**Objectif** : Vérifier que l'arrêt de l'application libère correctement tous les threads sans blocage.

**Procédure** : Fermer l'application ou interrompre l'exécution (Ctrl+C dans le terminal).

**Résultats attendus** :
- Tous les threads doivent se terminer rapidement
- Aucun thread ne doit rester bloqué indéfiniment
- Aucune fuite de mémoire (tous les vélos doivent être correctement libérés)

### Test de cohérence des données

**Objectif** : Vérifier que le nombre total de vélos reste constant dans le système.

**Procédure** : Additionner régulièrement le nombre de vélos dans tous les sites, le dépôt et les cargos (usagers en déplacement + camionnette).

**Résultat attendu** : Le total doit toujours être égal à NB_BIKES (35 vélos).

### Test de non-interblocage avec la camionnette

**Objectif** : S'assurer qu'aucun interblocage ne survient entre les usagers et la camionnette.

**Procédure** : Observer le système pendant une période prolongée (>10 minutes) en surveillant tout arrêt suspect de l'activité.

**Points critiques** :
- La camionnette et les usagers accèdent aux mêmes stations
- L'ordre d'acquisition des verrous doit être cohérent
- Les opérations par lot de la camionnette ne doivent pas monopoliser les ressources

### Tests de cas limites

**Cas à tester** :
1. **Tous les usagers préfèrent le même type** : Modifier temporairement le code pour forcer une préférence unique et observer la répartition
2. **Capacité minimale** : Réduire BORNES à 2 ou 3 pour augmenter la contention
3. **Nombre d'usagers élevé** : Augmenter NBPEOPLE à 20 ou 30 pour stresser le système
4. **Vélos en nombre limité** : Réduire NB_BIKES pour créer davantage de situations de pénurie

## Éléments manquants ou améliorations possibles

**IMPORTANT** : Voici les points qui pourraient nécessiter votre attention :

### 1. Statistiques et métriques
Il manque des mécanismes de mesure pour évaluer quantitativement les performances :
- Temps d'attente moyen des usagers
- Taux d'utilisation des vélos
- Nombre de déplacements effectués
- Efficacité de la redistribution par la camionnette

### 2. Tests automatisés
Le rapport ne mentionne pas de tests unitaires ou d'intégration automatisés. Il serait utile d'ajouter :
- Des tests vérifiant les invariants du système
- Des tests de charge automatiques
- Des assertions pour détecter les violations de cohérence

### 3. Stratégie de priorité
L'implémentation actuelle traite tous les usagers de manière équitable (FIFO), mais on pourrait envisager :
- Des mécanismes anti-famine plus sophistiqués
- Une priorité pour les usagers ayant attendu longtemps
- Une optimisation de la stratégie de la camionnette basée sur les statistiques d'utilisation

### 4. Gestion des préférences des usagers
Le système actuel force les usagers à attendre uniquement leur type préféré. Une amélioration pourrait être :
- Permettre un second choix après un certain délai d'attente
- Adapter dynamiquement les préférences selon la disponibilité

### 5. Documentation des scénarios de test
Le rapport devrait idéalement inclure :
- Des captures d'écran ou logs de l'interface pour illustrer les différents états testés
- Des tableaux récapitulatifs des résultats de tests
- Une matrice de traçabilité entre les exigences et les tests

### 6. Analyse des risques d'interblocage
Une analyse formelle pourrait être ajoutée :
- Graphe d'attente des ressources
- Preuve de l'absence de cycles dans les dépendances
- Démonstration que les 4 conditions de Coffman ne sont pas toutes réunies

### 7. Point d'attention dans l'implémentation actuelle

Dans le code fourni, il y a des commentaires TODO dans les méthodes **addBikes** et **getBikes** suggérant qu'elles pourraient être améliorées avec des variables de condition pour attendre si nécessaire. Actuellement, ces méthodes ne bloquent pas, ce qui est approprié pour la camionnette mais pourrait être limitant dans d'autres contextes d'utilisation.

---

**N'oubliez pas de :**
- Compléter vos noms d'auteurs en haut du document
- Convertir ce fichier en PDF nommé `rapport.pdf`
- Relire et vérifier la cohérence entre votre implémentation et les descriptions du rapport
- Ajouter vos propres observations et résultats de tests concrets avec des données chiffrées
- Documenter tout problème rencontré et résolu durant le développement
