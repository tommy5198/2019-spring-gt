# Graph-Theory-Project
Project of course - Graph Theory (EE6622E) in National Cheng Kung University.

## Usage

```
$ make                                          # build program.
$ ./main.out <input_file> <output_dotfile>      # output_file will create with additional .dot suffix.
$ dot -Tpng <dotfile> -o <png_filename>         # optional: create png from output dotfile.
```

## Chinese postman problem 

Find a shortest closed path or circuit that visits every edge of a directed graph

### input format

```
input should be a file contains multiple lines,
each line contains three element A, B, L sperated by a whitespace
a line "A B L" mean A has a unidirectionally connect to B with cost L
A and B are alphabet string represented as vertex in grpah
L is an interger number and should not exceed 10^5 
```

### output format

#### stdout

```
output contain 2 lines
first line start with "routing result: ", follow by the path of postman.
second line start with "total cost: ", follow by the total cost of all edge in above path.
see example for more detail.

note: you probably will see some fake-mininet output, just ignore them.
```

#### png 

```
each edge have 2 value describe as (x) y, x means edge cost, y means the order of postman walk.
```

### example

```
routing result: a -> b -> c -> d -> b -> c -> e -> f -> c -> a
total cost: 38
```

![](/test4.png)

## Algorithm description
    
首先讀取測資並檢查是否為強連通，透過遍歷每個點對確認是否有相互連通

```C++
nm = new NetworkManager();
nm->interpret(argv[1]);
Path *sp = new Path();
sp->append(nm->elist);
Vertex *vlst = nm->get_all_nodes();
bool isSCC = true;
for(Vertex *a = vlst; isSCC && a != NULL; a = a->next)
    for(Vertex *b = a->next; isSCC && b != NULL; b = b->next) 
        if(sp->find_paths(a->name, b->name).empty() || sp->find_paths(b->name, a->name).empty())
            isSCC = false;
```

接著記錄出入邊數不同的點，透過遍歷每個邊並取出其頭尾，為了之後輸出與計算方便這裡將 flowval 修改為邊成本 cap 改為 1，

```C++
map<Vertex *, int> diff; 

for(Edge *tmp = nm->elist; tmp; tmp = tmp->next) {
    diff[tmp->head]--;
    diff[tmp->tail]++;
    tmp->flowval = tmp->cap;
    tmp->cap = 1;
    vlist[tmp->head].push_back(tmp);
}
```

將出邊較多與入邊較多的點分群，並建立最小權最大流網路，接著加入源點與終點

```C++
vector<Vertex *> plusNode;
vector<Vertex *> minusNode; 
NetworkManager *mcmf = new NetworkManager();
mcmf->add_switch("src");
mcmf->add_switch("dst");
```

將分群後的點加入 mcmf 網路裡，建立源點到入邊較多的點，成本(flowval)設為 0 容量設為其出入邊數差，同時建立反向邊(容量為 0)以提供迴流

```C++
Edge *tmp = new Edge();
if(curDiff > 0) {
    plusNode.push_back(cur);
    tmp->link(src, cur);
    ...
    tmp->set_cap(curDiff);
    tmp->set_flowval(0);
    mcmf->add_edge(tmp);
    tmp = new Edge();
    tmp->link(cur, src);
    adj[cur].push_back(src);
    tmp->set_cap(0);
    tmp->set_flowval(0);
    mcmf->add_edge(tmp);
```

同時分群後入邊數較多的點要建立邊連向出邊數較多的點，其成本為點對最短路徑成本，同時紀錄最短路徑實際經過的點與邊成本

```C++
for (auto nt = minusNode.begin(); nt != minusNode.end(); ++nt) {
    Vertex *sec = *nt;
    sp->find_paths(cur->name, sec->name);
    int minL = MAX;
    for(int i = 0; i < sp->paths.size(); i++) {
        int tmpL = 0;
        vector<pair<string, int>> v;
        v.push_back(make_pair(cur->name, 0));
        for(int j = 0; j < sp->paths.at(i).size(); j++) {
            tmpL += sp->paths.at(i).at(j)->flowval;
            v.push_back(make_pair(sp->paths[i][j]->tail->name, sp->paths[i][j]->flowval));
        }
        if(minL > tmpL) {
            spMap[cur->name][sec->name].assign(v.begin(), v.end());
            minL = tmpL;
        }
    }
    Edge *intra = new Edge();
    intra->link(cur, sec);
    adj[cur].push_back(sec);
    intra->set_cap(MAX);
    intra->set_flowval(minL);
    mcmf->add_edge(intra);
    intra = new Edge();
    intra->link(sec, cur);
    adj[sec].push_back(cur);
    intra->set_cap(0);
    intra->set_flowval(-minL);
    mcmf->add_edge(intra);
}
```

出邊數較多的點也重複以上步驟，不同的是這些點會建立到終點的邊而不是源點

```C++
if(curDiff < 0) {
    minusNode.push_back(cur); 
    tmp->link(cur, dst);
    adj[cur].push_back(dst);
    ...
```

使用建立好的 mcmf 模型，首先尋找擴充路徑，並將路徑記錄下來

```C++
Q.push(src);

while(!Q.empty()) {
    Vertex *a = Q.front();
    Q.pop();
    inQ.erase(a);
    for (auto iter = adj[a].begin(); iter != adj[a].end(); ++iter) {
        Vertex *b = *iter;
        Edge *e = mcmf->get_edge(a, b);
        if(e->cap > 0 && d[a] + e->flowval < d[b]) {
            d[b] = d[a] + e->flowval;
            extraPath[b] = a;
            if(inQ.find(b) == inQ.end()) {
                Q.push(b);
                inQ.insert(b);
            }
        }
    } 
}
if(d[dst] == MAX) break;
```

依據找到的擴充路徑更新邊的流量，此流量也可表示為需要走此路徑幾次

```C++
int df = MAX;
for(Vertex *a = dst; a != src; a = extraPath[a]) {
    Edge *e = mcmf->get_edge(extraPath[a], a);
    df = df < e->cap ? df : e->cap;
}
for(Vertex *a = dst; a != src; a = extraPath[a]) {
    Edge *e = mcmf->get_edge(extraPath[a], a);
    e->cap -= df;
    e = mcmf->get_edge(a, extraPath[a]);
    e->cap += df;
}
```

跑完 mcmf 模型後取得匹配的邊並加回原圖(nm)，這裡將新加入的邊成本設為負數用以辨認是否為原圖的邊

```C++
for (auto iter = plusNode.begin(); iter != plusNode.end(); ++iter) {
    for (auto til = minusNode.begin(); til != minusNode.end(); ++til) {
        Edge *e = mcmf->get_edge(*iter, *til);
        if(e->cap != MAX) {
            e->cap = MAX - e->cap;
            e->flowval = -e->flowval;
            nm->add_edge(e);
            vlist[e->head].push_back(e);
        }           
    }  
} 
```

接著在更新過後的原圖尋找尤拉迴路，每個邊走過一次 cap 會減一並記錄其走的順序，

```C++
findEulerCircuit(nm->elist->head);

void findEulerCircuit(Vertex *x)
{
    for(int i = 0; i < vlist[x].size(); i++) {
        Edge *e = vlist[x][i];
        if(e && e->cap) {
            e->cap--;
            findEulerCircuit(e->tail);
            resultEdge.push_front(e);            
        }
    }
}
```

最後將記錄好的邊一一取出計算其成本，其中新加入的邊(flowval < 0)要拆開成對應的最短路徑

```C++
for (auto iter = resultEdge.begin(); iter != resultEdge.end(); ++iter)
    if(e->flowval > 0) {
        cout << " -> " << e->tail->name;
        totCost += e->flowval;
        if(resultEdgeHead == NULL) {
            resultEdgeHead = new Edge(e);
            resultEdgeList = resultEdgeHead;
        } else {
            resultEdgeList->next = new Edge(e);
            resultEdgeList = resultEdgeList->next;
        }
        resultEdgeList->cap = cnt++;
    } else {
        for(int i = 1; i < spMap[e->head->name][e->tail->name].size(); i++) {
            string zz = spMap[e->head->name][e->tail->name][i].first;
            int zLen = spMap[e->head->name][e->tail->name][i].second;
            cout << " -> " << zz;
            ...
            int zLen = spMap[e->head->name][e->tail->name][i].second
            ...
            resultEdgeList->cap = cnt++;
            resultEdgeList->flowval = zLen;
        }
        totCost -= e->flowval;
    }

```

將結果輸出至 dotfile

```C++
Gplot *gp = new Gplot();
gp->gp_add(resultEdgeHead);
gp->gp_dump(true);
gp->gp_export(argv[2]);
 ```
