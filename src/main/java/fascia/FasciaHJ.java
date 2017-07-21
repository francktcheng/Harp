package fascia;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * Java version of fascia shared-memory mode http://fascia-psu.sourceforge.net
 *
 * In order to refer to the original C++ version directly, the code doesn't follow the JAVA convention.
 */
public class FasciaHJ {

    /**
     * @brief read in graph data in pairs of edges
     *
     * @param g
     * @param graph_file
     * @param labeled
     *
     * @return 
     */
    private void read_in_graph(Graph g, String graph_file, boolean labeled){

        try{
            Path graph_path = Paths.get(graph_file);
            InputStream in = Files.newInputStream(graph_path);
            BufferedReader reader = new BufferedReader(new InputStreamReader(in));

            int n_g, m_g;

            //get number of nodes (vertice)
            n_g = Integer.parseInt( reader.readLine() );
            //get number of edges
            m_g = Integer.parseInt( reader.readLine() );

            int[] srcs_g = new int[m_g];
            int[] dsts_g = new int[m_g];

            int[] labels_g;

            // what are those labels ?
            if( labeled){
                labels_g = new int[n_g];
                for(int i = 0; i < n_g; ++i){
                    labels_g[i] = Integer.parseInt( reader.readLine() );
                }
            }else{
                labels_g = null;
            }

            for(int i = 0; i < m_g; ++i){

                String[] src_dst = reader.readLine().split("\\s+");
                srcs_g[i] = Integer.parseInt(src_dst[0]  );
                dsts_g[i] = Integer.parseInt( src_dst[1] );

            }

            in.close();

            g.init(n_g, m_g, srcs_g, dsts_g, labels_g, labeled);

            srcs_g = null;
            dsts_g = null;

        } catch (IOException x) {
            System.err.println(x);
        }

    }

    public FasciaHJ(String[] args)  {
        FasciaOptions foption = new FasciaOptions(args);

        String graph_file = foption.graph_file;
        String template_file = foption.template_file;
        String batch_file = foption.batch_file;
        int iterations = foption.iterations;
        boolean do_outerloop = foption.do_outerloop;
        boolean calculate_automorphism = foption.calculate_automorphism;
        boolean labeled = foption.labeled;
        boolean do_gdd = foption.do_gdd;
        boolean do_vert = foption.do_vert;
        boolean verbose = foption.verbose;
        int motif = foption.motif;
        boolean timing = foption.timing;
        Constants.THREAD_NUM = foption.thread_num;
        Constants.CORE_NUM = foption.core_num;
        Constants.THD_AFFINITY = foption.thd_affinity;

        if(motif != 0){
            System.err.println("run_motif skipped");
        }else if( template_file != null){
            run_single(graph_file, template_file, labeled, do_vert, do_gdd, iterations,
                    do_outerloop, calculate_automorphism, verbose, timing);
        }else if (batch_file != null){
            System.err.println("run_batch skipped");
        }
    }

    private void run_single(String graph_file, String template_file, boolean labeled, boolean do_vert, boolean do_gdd,
                            int iterations, boolean do_outerloop, boolean calculate_automorphism, boolean verbose, boolean timing)  {

        //graph data
        Graph g = new Graph();
        //template date
        Graph t = new Graph();

        // String vert_file;
        // String gdd_file;
        //
        // if( do_vert){
        //     vert_file = template_file + ".vert";
        //     System.err.println("do_vert skipped");
        // }
        // if(do_gdd ){
        //     gdd_file = template_file + ".gdd";
        //     System.err.println("do_gdd skipped");
        // }

        //input file has pairs of edges in graph
        long readbegin = System.currentTimeMillis();
        read_in_graph(g, graph_file, labeled);
        read_in_graph(t, template_file, labeled);

	    System.out.println("Loading graph takes: " + (System.currentTimeMillis()-readbegin) + "ms");

        long elt = System.currentTimeMillis();

        double full_count = 0.0;

        if( do_outerloop){
            System.err.println("outerloop mode skipped");
        }else{

            colorcount_HJ graph_count = new colorcount_HJ();
            graph_count.init(g, calculate_automorphism, do_gdd, do_vert, verbose);
            full_count += graph_count.do_full_count(t, iterations);

            if( do_gdd || do_vert){
                //do_gdd or do_vert skipped
            }
        }

        System.out.println("Count: " + full_count);
        elt = System.currentTimeMillis() - elt;
        System.out.println("Total time (real computation):" + elt + "ms" );

    }

    public static void main(String[] args)  {

        long begin = System.currentTimeMillis();
        FasciaHJ fascia = new FasciaHJ(args);
        System.out.println("Total time (from beginning to end): " + (System.currentTimeMillis()-begin) + "ms");

    }
}
