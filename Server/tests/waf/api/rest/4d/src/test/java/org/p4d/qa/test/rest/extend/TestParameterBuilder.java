/**
 * 
 */
package org.p4d.qa.test.rest.extend;

import org.apache.http.HttpRequest;
import org.apache.http.NameValuePair;
import org.wakanda.qa.commons.server.rest.RESTRequestBuilder;

public abstract class TestParameterBuilder {

	private int size;
	private String expectedIndex;

	protected abstract String getTable(int i);

	protected String getEntitySet(int i) {
		return null;
	}

	protected String getKey(int i) {
		return null;
	}

	protected String[] getAttributes(int i) {
		return null;
	}



	protected NameValuePair[] getParams(int i) {
		return null;
	}

	protected HttpRequest getRequest(int i) {
		RESTRequestBuilder rb = new RESTRequestBuilder();
		HttpRequest request = null;

		try {
			rb.addDataClass(getTable(i));
			rb.addKey(getKey(i));
			rb.addAttributes(getAttributes(i));
			rb.addEntitySet(getEntitySet(i));
			rb.addParameters(getParams(i));
			request = rb.buildRequest();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return request;
	}

	protected String getIndex() {
		return expectedIndex;
	}

	public TestParameterBuilder(int size, String expectedIndex) {
		super();
		this.size = size;
		this.expectedIndex = expectedIndex;
	}

	public TestParam[] buildParams() throws Exception {

		TestParam[] result = new TestParam[size];
		for (int i = 0; i < size; i++) {

			HttpRequest request = getRequest(i);
			String expected = getIndex() + "-" + (i + 1);

			result[i] = new TestParam(request, expected);
		}
		return result;
	}
}